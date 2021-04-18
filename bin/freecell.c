/* Copyright (C) 2019, 2021  June McEnroe <june@causal.agency>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

typedef unsigned uint;
typedef unsigned char byte;

typedef byte Card;
enum {
	A = 1,
	J = 11,
	Q = 12,
	K = 13,
	Rank = 0x0F,
	Suit = 0x30,
	Color = 0x10,
	Club = 0x00,
	Diamond = 0x10,
	Spade = 0x20,
	Heart = 0x30,
};

enum { StackCap = 52 };
struct Stack {
	byte len;
	Card cards[StackCap];
};
static void push(struct Stack *stack, Card card) {
	assert(stack->len < StackCap);
	stack->cards[stack->len++] = card;
}
static Card pop(struct Stack *stack) {
	if (!stack->len) return 0;
	return stack->cards[--stack->len];
}
static Card peek(struct Stack *stack) {
	if (!stack->len) return 0;
	return stack->cards[stack->len-1];
}

enum {
	Foundation,
	Cell = Foundation + 4,
	Tableau = Cell + 4,
	Stacks = Tableau + 8,
};
static struct Stack stacks[Stacks];

struct Move {
	byte dst;
	byte src;
};

enum { QCap = 16 };
static struct {
	struct Move moves[QCap];
	uint r, w, u;
} q;
static void enq(byte dst, byte src) {
	q.moves[q.w % QCap].dst = dst;
	q.moves[q.w % QCap].src = src;
	q.w++;
}
static void deq(void) {
	struct Move move = q.moves[q.r++ % QCap];
	push(&stacks[move.dst], pop(&stacks[move.src]));
}
static void undo(void) {
	uint len = q.w - q.u;
	if (!len || len > QCap) return;
	for (uint i = len-1; i < len; --i) {
		struct Move move = q.moves[(q.u+i) % QCap];
		push(&stacks[move.src], pop(&stacks[move.dst]));
	}
	q.r = q.w = q.u;
}

// https://rosettacode.org/wiki/Deal_cards_for_FreeCell
static uint lcgState;
static uint lcg(void) {
	lcgState = (214013 * lcgState + 2531011) % (1 << 31);
	return lcgState >> 16;
}
static void deal(uint game) {
	lcgState = game;
	struct Stack deck = {0};
	for (Card i = A; i <= K; ++i) {
		push(&deck, Club | i);
		push(&deck, Diamond | i);
		push(&deck, Heart | i);
		push(&deck, Spade | i);
	}
	for (uint stack = 0; deck.len; ++stack) {
		uint i = lcg() % deck.len;
		Card card = deck.cards[i];
		deck.cards[i] = deck.cards[--deck.len];
		push(&stacks[Tableau + stack%8], card);
	}
}

static bool win(void) {
	for (uint i = Foundation; i < Cell; ++i) {
		if (stacks[i].len != 13) return false;
	}
	return true;
}

static bool valid(uint dst, Card card) {
	Card top = peek(&stacks[dst]);
	if (dst < Cell) {
		if (!top) return (card & Rank) == A;
		return (card & Suit) == (top & Suit)
			&& (card & Rank) == (top & Rank) + 1;
	}
	if (!top) return true;
	if (dst >= Tableau) {
		return (card & Color) != (top & Color)
			&& (card & Rank) == (top & Rank) - 1;
	}
	return false;
}

static void autoEnq(void) {
	Card min[] = { K, K };
	for (uint i = Cell; i < Stacks; ++i) {
		for (uint j = 0; j < stacks[i].len; ++j) {
			Card card = stacks[i].cards[j];
			if ((card & Rank) < min[!!(card & Color)]) {
				min[!!(card & Color)] = card & Rank;
			}
		}
	}
	for (uint src = Cell; src < Stacks; ++src) {
		Card card = peek(&stacks[src]);
		if (!card) continue;
		if (min[!(card & Color)] < (card & Rank)-1) continue;
		for (uint dst = Foundation; dst < Cell; ++dst) {
			if (valid(dst, card)) {
				enq(dst, src);
				return;
			}
		}
	}
}

static void moveSingle(uint dst, uint src) {
	if (!valid(dst, peek(&stacks[src]))) return;
	q.u = q.w;
	enq(dst, src);
}

static uint freeCells(uint cells[static Stacks], uint dst) {
	uint len = 0;
	for (uint i = Cell; i < Stacks; ++i) {
		if (i == dst) continue;
		if (!stacks[i].len) cells[len++] = i;
	}
	return len;
}

static uint moveDepth(uint src) {
	struct Stack stack = stacks[src];
	if (stack.len < 2) return stack.len;
	uint n = 1;
	for (uint i = stack.len-2; i < stack.len; --i, ++n) {
		if ((stack.cards[i] & Color) == (stack.cards[i+1] & Color)) break;
		if ((stack.cards[i] & Rank) != (stack.cards[i+1] & Rank) + 1) break;
	}
	return n;
}

static void moveColumn(uint dst, uint src) {
	uint depth;
	uint cells[Stacks];
	uint free = freeCells(cells, dst);
	for (depth = moveDepth(src); depth; --depth) {
		if (free < depth-1) continue;
		if (valid(dst, stacks[src].cards[stacks[src].len-depth])) break;
	}
	if (depth < 2 || dst < Tableau) {
		moveSingle(dst, src);
		return;
	}
	q.u = q.w;
	for (uint i = 0; i < depth-1; ++i) {
		enq(cells[i], src);
	}
	enq(dst, src);
	for (uint i = depth-2; i < depth-1; --i) {
		enq(dst, cells[i]);
	}
}

static void curse(void) {
	setlocale(LC_CTYPE, "");
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	start_color();
	use_default_colors();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_RED, COLOR_WHITE);
	init_pair(3, COLOR_GREEN, -1);
}

static void drawCard(bool hi, int y, int x, Card card) {
	if (!card) return;
	move(y, x);
	attr_set(hi ? A_REVERSE : A_NORMAL, (card & Color) ? 2 : 1, NULL);
	switch (card & Rank) {
		break; case A: addstr("A ");
		break; case 10: addstr("10");
		break; case J: addstr("J ");
		break; case Q: addstr("Q ");
		break; case K: addstr("K ");
		break; default: {
			addch('0' + (card & Rank));
			addch(' ');
		}
	}
	switch (card & Suit) {
		break; case Club: addstr("\u2663");
		break; case Diamond: addstr("\u2666");
		break; case Spade: addstr("\u2660");
		break; case Heart: addstr("\u2665");
		break; default:;
	}
	attr_set(A_NORMAL, 0, NULL);
}

static void drawStack(bool hi, int y, int x, const struct Stack *stack) {
	for (uint i = 0; i < stack->len; ++i) {
		drawCard(hi && i == stack->len-1, y++, x, stack->cards[i]);
	}
}

enum {
	Padding = 1,
	CardWidth = 3,
	CardHeight = 1,
	CellX = Padding,
	CellY = 2*CardHeight,
	FoundationX = CellX + 4*(CardWidth+Padding),
	FoundationY = CellY,
	TableauX = CellX,
	TableauY = CellY + 2*CardHeight,
};

static uint game;
static uint srcStack = Stacks;

static void draw(void) {
	erase();
	static char buf[256];
	if (!buf[0]) snprintf(buf, sizeof(buf), "Game #%u", game);
	attr_set(A_NORMAL, 3, NULL);
	mvaddstr(0, Padding, buf);
	for (uint i = 0; i < Stacks; ++i) {
		int y, x;
		char key;
		if (i < Cell) {
			y = FoundationY;
			x = FoundationX + (i-Foundation) * (CardWidth+Padding);
			key = '_';
		} else if (i < Tableau) {
			y = CellY;
			x = CellX + (i-Cell) * (CardWidth+Padding);
			key = '1' + i-Cell;
		} else {
			y = TableauY;
			x = TableauX + (i-Tableau) * (CardWidth+Padding);
			key = "QWERASDF"[i-Tableau];
		}
		if (i < Tableau) {
			mvaddch(y, x+1, COLOR_PAIR(3) | key);
		} else {
			mvaddch(y + 8*CardHeight, x+1, COLOR_PAIR(3) | key);
		}
		if (i < Cell) {
			drawCard(false, y, x, peek(&stacks[i]));
		} else {
			drawStack(i == srcStack, y, x, &stacks[i]);
		}
	}
}

static void input(void) {
	char ch = getch();
	uint stack = Stacks;
	switch (tolower(ch)) {
		break; case '\33': srcStack = Stacks;
		break; case 'u': case '\b': case '\177': undo();
		break; case '1': case '!': stack = Cell+0;
		break; case '2': case '@': stack = Cell+1;
		break; case '3': case '#': stack = Cell+2;
		break; case '4': case '$': stack = Cell+3;
		break; case '_': case ' ': stack = Foundation;
		break; case 'q': stack = Tableau+0;
		break; case 'w': stack = Tableau+1;
		break; case 'e': stack = Tableau+2;
		break; case 'r': stack = Tableau+3;
		break; case 'a': stack = Tableau+4;
		break; case 's': stack = Tableau+5;
		break; case 'd': stack = Tableau+6;
		break; case 'f': stack = Tableau+7;
	}
	if (stack == Stacks) return;

	if (srcStack < Stacks) {
		Card card = peek(&stacks[srcStack]);
		if (stack == Foundation) {
			for (; stack < Cell; ++stack) {
				if (valid(stack, card)) break;
			}
			if (stack == Cell) return;
		}
		if (stack == srcStack) {
			for (stack = Cell; stack < Stacks; ++stack) {
				if (!stacks[stack].len) break;
			}
			if (stack == Stacks) return;
		}
		if (isupper(ch)) {
			moveSingle(stack, srcStack);
		} else {
			moveColumn(stack, srcStack);
		}
		srcStack = Stacks;

	} else if (stack >= Cell && stacks[stack].len) {
		srcStack = stack;
	}
}

static void status(void) {
	printf("Game #%u %s!\n", game, win() ? "win" : "lose");
}

int main(int argc, char *argv[]) {
	game = 1 + time(NULL) % 32000;
	uint delay = 50;
	for (int opt; 0 < (opt = getopt(argc, argv, "d:n:"));) {
		switch (opt) {
			break; case 'd': delay = strtoul(optarg, NULL, 10);
			break; case 'n': game = strtoul(optarg, NULL, 10);
			break; default:  return EX_USAGE;
		}
	}
	curse();
	deal(game);
	atexit(status);
	while (!win()) {
		while (q.r < q.w) {
			deq();
			draw();
			refresh();
			usleep(delay * 1000);
			if (q.r == q.w) autoEnq();
		}
		draw();
		input();
	}
	endwin();
}
