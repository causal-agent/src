/* Copyright (C) 2018  June McEnroe <june@causal.agency>
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

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

struct Span {
	char *ptr;
	size_t len;
};

static struct Span spanBefore(struct Span span, size_t index) {
	return (struct Span) { span.ptr, index };
}
static struct Span spanAfter(struct Span span, size_t index) {
	return (struct Span) { span.ptr + index, span.len - index };
}

static struct Table {
	struct Table *next;
	struct Table *prev;
	size_t len;
	struct Span span[];
} *table;

static struct Table *tableNew(size_t cap) {
	struct Table *next = malloc(sizeof(*next) + cap * sizeof(struct Span));
	if (!next) err(EX_OSERR, "malloc");
	next->next = NULL;
	next->prev = NULL;
	next->len = 0;
	return next;
}

static struct Table *tableSpan(struct Span span) {
	struct Table *next = tableNew(1);
	next->len = 1;
	next->span[0] = span;
	return next;
}

static void tableUndo(void) {
	if (table->prev) table = table->prev;
}
static void tableRedo(void) {
	if (table->next) table = table->next;
}

static void tableDiscard(void) {
	struct Table *it = table->next;
	table->next = NULL;
	while (it) {
		struct Table *next = it->next;
		free(it);
		it = next;
	}
}

static void tablePush(struct Table *next) {
	tableDiscard();
	table->next = next;
	next->prev = table;
	table = next;
}

static struct Span *tableInsert(size_t atStart, struct Span span) {
	// TODO: Handle table->len == 0
	struct Table *next = tableNew(table->len + 2);
	struct Span *insert = NULL;

	size_t start = 0;
	struct Span *nextSpan = next->span;
	for (size_t i = 0; i < table->len; ++i) {
		struct Span prevSpan = table->span[i];
		size_t end = start + prevSpan.len;

		if (atStart > start && atStart < end) {
			insert = nextSpan + 1;
			*nextSpan++ = spanBefore(prevSpan, atStart - start);
			*nextSpan++ = span;
			*nextSpan++ = spanAfter(prevSpan, atStart - start);
		} else if (atStart == start) {
			insert = nextSpan;
			*nextSpan++ = span;
			*nextSpan++ = prevSpan;
		} else {
			*nextSpan++ = prevSpan;
		}

		start = end;
	}
	next->len = nextSpan - next->span;

	tablePush(next);
	return insert;
}

static void tableDelete(size_t atStart, size_t atEnd) {
	struct Table *next = tableNew(table->len + 1);

	size_t start = 0;
	struct Span *nextSpan = next->span;
	for (size_t i = 0; i < table->len; ++i) {
		struct Span prevSpan = table->span[i];
		size_t end = start + prevSpan.len;

		if (atStart > start && atEnd < end) {
			*nextSpan++ = spanBefore(prevSpan, atStart - start);
			*nextSpan++ = spanAfter(prevSpan, atEnd - start);
		} else if (atStart > start && atStart < end && atEnd >= end) {
			*nextSpan++ = spanBefore(prevSpan, atStart - start);
		} else if (atStart <= start && atEnd < end && atEnd > start) {
			*nextSpan++ = spanAfter(prevSpan, atEnd - start);
		} else if (atStart <= start && atEnd >= end) {
		} else {
			*nextSpan++ = prevSpan;
		}

		start = end;
	}
	next->len = nextSpan - next->span;

	tablePush(next);
}

static void curse(void) {
	setlocale(LC_CTYPE, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, true);
	set_escdelay(100);
}

static void draw(void) {
	move(0, 0);
	for (size_t i = 0; i < table->len; ++i) {
		addnstr(table->span[i].ptr, (int)table->span[i].len);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) return EX_USAGE;

	const char *path = argv[1];
	int fd = open(path, O_RDWR | O_CREAT, 0644);
	if (fd < 0) err(EX_CANTCREAT, "%s", path);

	struct stat stat;
	int error = fstat(fd, &stat);
	if (error) err(EX_IOERR, "%s", path);

	if (stat.st_size) {
		char *file = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (file == MAP_FAILED) err(EX_IOERR, "%s", path);
		table = tableSpan((struct Span) { file, stat.st_size });
	} else {
		table = tableSpan((struct Span) { NULL, 0 });
	}

	curse();
	draw();
	getch();
	endwin();
}
