/* Copyright (C) 2019  June McEnroe <june@causal.agency>
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

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
	Cols = 80,
	Rows = 25,
};
static char page[Rows][Cols];

static char get(int y, int x) {
	if (y < 0 || y >= Rows) return 0;
	if (x < 0 || x >= Cols) return 0;
	return page[y][x];
}
static void put(int y, int x, char v) {
	if (y < 0 || y >= Rows) return;
	if (x < 0 || x >= Cols) return;
	page[y][x] = v;
}

enum { StackLen = 1024 };
static long stack[StackLen];
static size_t top = StackLen;

static void push(long val) {
	if (!top) errx(1, "stack overflow");
	stack[--top] = val;
}
static long pop(void) {
	if (top == StackLen) return 0;
	return stack[top++];
}

static struct {
	int y, x;
	int dy, dx;
} pc = { .dx = 1 };

static void inc(void) {
	pc.y += pc.dy;
	pc.x += pc.dx;
	if (pc.y == -1) pc.y += Rows;
	if (pc.x == -1) pc.x += Cols;
	if (pc.y == Rows) pc.y -= Rows;
	if (pc.x == Cols) pc.x -= Cols;
}

static bool string;

static bool step(void) {
	char ch = page[pc.y][pc.x];

	if (ch == '"') {
		string ^= true;
	} else if (string) {
		push(ch);
		inc();
		return true;
	}

	if (ch == '?') ch = "><^v"[rand() % 4];

	long x, y, v;
	switch (ch) {
		break; case '+': push(pop() + pop());
		break; case '-': y = pop(); x = pop(); push(x - y);
		break; case '*': push(pop() * pop());
		break; case '/': y = pop(); x = pop(); push(x / y);
		break; case '%': y = pop(); x = pop(); push(x % y);
		break; case '!': push(!pop());
		break; case '`': y = pop(); x = pop(); push(x > y);
		break; case '>': pc.dy = 0; pc.dx = +1;
		break; case '<': pc.dy = 0; pc.dx = -1;
		break; case '^': pc.dy = -1; pc.dx = 0;
		break; case 'v': pc.dy = +1; pc.dx = 0;
		break; case '_': pc.dy = 0; pc.dx = (pop() ? -1 : +1);
		break; case '|': pc.dx = 0; pc.dy = (pop() ? -1 : +1);
		break; case ':': x = pop(); push(x); push(x);
		break; case '\\': y = pop(); x = pop(); push(y); push(x);
		break; case '$': pop();
		break; case '.': printf("%ld ", pop()); fflush(stdout);
		break; case ',': printf("%c", (char)pop()); fflush(stdout);
		break; case '#': inc();
		break; case 'g': y = pop(); x = pop(); push(get(y, x));
		break; case 'p': y = pop(); x = pop(); v = pop(); put(y, x, v);
		break; case '&': x = EOF; scanf("%ld", &x); push(x);
		break; case '~': push(getchar());
		break; case '@': return false;
		break; default:  if (ch >= '0' && ch <= '9') push(ch - '0');
	}

	inc();
	return true;
}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	memset(page, ' ', sizeof(page));

	FILE *file = stdin;
	if (argc > 1) {
		file = fopen(argv[1], "r");
		if (!file) err(1, "%s", argv[1]);
	}

	int y = 0;
	char *line = NULL;
	size_t cap = 0;
	while (y < Rows && 0 < getline(&line, &cap, file)) {
		for (int x = 0; x < Cols; ++x) {
			if (line[x] == '\n' || !line[x]) break;
			page[y][x] = line[x];
		}
		y++;
	}
	free(line);

	while (step());
	return pop();
}
