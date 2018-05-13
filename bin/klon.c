/* Copyright (c) 2018, June McEnroe <programble@gmail.com>
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
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t Card;

enum {
	MASK_RANK   = 0x0F,
	MASK_SUIT   = 0x30,
	MASK_COLOR  = 0x10,
	MASK_UP     = 0x40,
	MASK_SELECT = 0x80,
};

enum {
	SUIT_CLUB    = 0x00,
	SUIT_DIAMOND = 0x10,
	SUIT_SPADE   = 0x20,
	SUIT_HEART   = 0x30,
};

struct Stack {
	Card data[52];
	uint8_t index;
};
#define EMPTY { .data = {0}, .index = 52 }

static void push(struct Stack *stack, Card card) {
	assert(stack->index > 0);
	stack->data[--stack->index] = card;
}

static Card pop(struct Stack *stack) {
	assert(stack->index < 52);
	return stack->data[stack->index++];
}

static Card get(const struct Stack *stack, uint8_t i) {
	if (stack->index + i > 51) return 0;
	return stack->data[stack->index + i];
}

static uint8_t len(const struct Stack *stack) {
	return 52 - stack->index;
}

struct State {
	struct Stack stock;
	struct Stack waste;
	struct Stack found[4];
	struct Stack table[7];
};

static struct State g = {
	.stock = {
		.data = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
			0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
			0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
			0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
			0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
		},
		.index = 0,
	},
	.waste = EMPTY,
	.found = { EMPTY, EMPTY, EMPTY, EMPTY },
	.table = {
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
	},
};

static struct State save;

static void checkpoint(void) {
	memcpy(&save, &g, sizeof(struct State));
}

static void undo(void) {
	memcpy(&g, &save, sizeof(struct State));
}

static void shuffle(void) {
	for (int i = 51; i > 0; --i) {
		int j = arc4random_uniform(i + 1);
		uint8_t x = g.stock.data[i];
		g.stock.data[i] = g.stock.data[j];
		g.stock.data[j] = x;
	}
}

static void deal(void) {
	for (int i = 0; i < 7; ++i) {
		for (int j = i; j < 7; ++j) {
			push(&g.table[j], pop(&g.stock));
		}
	}
}

static void reveal(void) {
	for (int i = 0; i < 7; ++i) {
		if (get(&g.table[i], 0)) {
			push(&g.table[i], pop(&g.table[i]) | MASK_UP);
		}
	}
}

static void draw(void) {
	if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
	if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
	if (get(&g.stock, 0)) push(&g.waste, pop(&g.stock) | MASK_UP);
}

static void wasted(void) {
	uint8_t n = len(&g.waste);
	for (int i = 0; i < n; ++i) {
		push(&g.stock, pop(&g.waste) & ~MASK_UP);
	}
}

static void transfer(struct Stack *dest, struct Stack *src, uint8_t n) {
	struct Stack temp = EMPTY;
	for (int i = 0; i < n; ++i) {
		push(&temp, pop(src));
	}
	for (int i = 0; i < n; ++i) {
		push(dest, pop(&temp));
	}
}

static bool canFound(const struct Stack *found, Card card) {
	if (!get(found, 0)) return (card & MASK_RANK) == 1;
	if ((card & MASK_SUIT) != (get(found, 0) & MASK_SUIT)) return false;
	return (card & MASK_RANK) == (get(found, 0) & MASK_RANK) + 1;
}

static bool canTable(const struct Stack *table, Card card) {
	if (!get(table, 0)) return (card & MASK_RANK) == 13;
	if ((card & MASK_COLOR) == (get(table, 0) & MASK_COLOR)) return false;
	return (card & MASK_RANK) == (get(table, 0) & MASK_RANK) - 1;
}

enum {
	PAIR_EMPTY = 1,
	PAIR_BACK,
	PAIR_BLACK,
	PAIR_RED,
};

static void curse(void) {
	setlocale(LC_CTYPE, "");

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, true);
	set_escdelay(100);
	curs_set(0);

	start_color();
	assume_default_colors(-1, -1);
	init_pair(PAIR_EMPTY, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_BACK,  COLOR_WHITE, COLOR_BLUE);
	init_pair(PAIR_BLACK, COLOR_BLACK, COLOR_WHITE);
	init_pair(PAIR_RED,   COLOR_RED,   COLOR_WHITE);
}

static const char rank[] = "\0A23456789TJQK";
static const char *suit[] = {
	[SUIT_HEART]   = "♥",
	[SUIT_CLUB]    = "♣",
	[SUIT_DIAMOND] = "♦",
	[SUIT_SPADE]   = "♠",
};

static void renderCard(int y, int x, Card card) {
	if (card & MASK_UP) {
		bkgdset(
			COLOR_PAIR(card & MASK_COLOR ? PAIR_RED : PAIR_BLACK)
			| (card & MASK_SELECT ? A_REVERSE : A_NORMAL)
		);

		move(y, x);
		addch(rank[card & MASK_RANK]);
		addstr(suit[card & MASK_SUIT]);
		addch(' ');

		move(y + 1, x);
		addstr(suit[card & MASK_SUIT]);
		addch(' ');
		addstr(suit[card & MASK_SUIT]);

		move(y + 2, x);
		addch(' ');
		addstr(suit[card & MASK_SUIT]);
		addch(rank[card & MASK_RANK]);

	} else {
		bkgdset(COLOR_PAIR(card ? PAIR_BACK : PAIR_EMPTY));
		mvaddstr(y, x, "   ");
		mvaddstr(y + 1, x, "   ");
		mvaddstr(y + 2, x, "   ");
	}
}

static void render(void) {
	bkgdset(COLOR_PAIR(0));
	erase();

	int x = 2;
	int y = 1;

	renderCard(y, x, get(&g.stock, 0));

	x += 5;
	renderCard(y, x++, get(&g.waste, 2));
	renderCard(y, x++, get(&g.waste, 1));
	renderCard(y, x, get(&g.waste, 0));

	x += 5;
	for (int i = 0; i < 4; ++i) {
		renderCard(y, x, get(&g.found[i], 0));
		x += 4;
	}

	x = 2;
	for (int i = 0; i < 7; ++i) {
		y = 5;
		renderCard(y, x, 0);
		for (int j = len(&g.table[i]); j > 0; --j) {
			renderCard(y, x, get(&g.table[i], j - 1));
			y++;
		}
		x += 4;
	}
}

static struct {
	struct Stack *stack;
	uint8_t depth;
} input;

static void deepen(void) {
	assert(input.stack);
	if (input.depth == len(input.stack)) return;
	if (!(get(input.stack, input.depth) & MASK_UP)) return;
	input.stack->data[input.stack->index + input.depth] |= MASK_SELECT;
	input.depth++;
}

static void select(struct Stack *stack) {
	if (!get(stack, 0)) return;
	input.stack = stack;
	input.depth = 0;
	deepen();
}

static void commit(struct Stack *dest) {
	assert(input.stack);
	for (int i = 0; i < input.depth; ++i) {
		input.stack->data[input.stack->index + i] &= ~MASK_SELECT;
	}
	if (dest) {
		checkpoint();
		transfer(dest, input.stack, input.depth);
	}
	input.stack = NULL;
	input.depth = 0;
}

int main() {
	curse();

	shuffle();
	deal();
	checkpoint();

	for (;;) {
		reveal();
		render();

		int c = getch();
		if (!input.stack) {
			if (c == 'q') {
				break;
			} else if (c == 'u') {
				undo();
			} else if (c == 's' || c == ' ') {
				if (get(&g.stock, 0)) {
					checkpoint();
					draw();
				} else {
					wasted();
				}
			} else if (c == 'w') {
				select(&g.waste);
			} else if (c >= 'a' && c <= 'd') {
				select(&g.found[c - 'a']);
			} else if (c >= '1' && c <= '7') {
				select(&g.table[c - '1']);
			}

		} else {
			if (c >= '1' && c <= '7') {
				struct Stack *table = &g.table[c - '1'];
				if (input.stack == table) {
					deepen();
				} else if (canTable(table, get(input.stack, input.depth - 1))) {
					commit(table);
				} else {
					commit(NULL);
				}
			} else if (input.depth == 1 && c >= 'a' && c <= 'd') {
				struct Stack *found = &g.found[c - 'a'];
				if (canFound(found, get(input.stack, 0))) {
					commit(found);
				} else {
					commit(NULL);
				}
			} else if (input.depth == 1 && (c == 'f' || c == '\n')) {
				struct Stack *found;
				for (int i = 0; i < 4; ++i) {
					found = &g.found[i];
					if (canFound(found, get(input.stack, 0))) break;
					found = NULL;
				}
				commit(found);
			} else {
				commit(NULL);
			}
		}
	}

	endwin();
	return 0;
}
