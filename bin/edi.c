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

#define _XOPEN_SOURCE_EXTENDED

#include <curses.h>
#include <err.h>
#include <locale.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static struct Span {
	size_t at, to;
} Span(size_t at, size_t to) {
	return (struct Span) { at, to };
}

static bool spanEqual(struct Span a, struct Span b) {
	return a.at == b.at && a.to == b.to;
}
static bool spanIn(struct Span a, struct Span b) {
	return a.at > b.at && a.to < b.to;
}
static bool spanHeadIn(struct Span a, struct Span b) {
	return a.at > b.at && a.at < b.to;
}
static bool spanTailIn(struct Span a, struct Span b) {
	return a.to > b.at && a.to < b.to;
}

static struct Vec {
	char *ptr;
	size_t len;
} Vec(char *ptr, size_t len) {
	return (struct Vec) { ptr, len };
}

static struct Vec vecHead(struct Vec vec, struct Span span, size_t at) {
	return Vec(vec.ptr, at - span.at);
}
static struct Vec vecTail(struct Vec vec, struct Span span, size_t at) {
	return Vec(vec.ptr + (at - span.at), vec.len - (at - span.at));
}

static struct Table {
	struct Table *next, *prev;
	size_t len;
	struct Vec vec[];
} *Table(size_t cap) {
	size_t size = sizeof(struct Table) + cap * sizeof(struct Vec);
	struct Table *table = malloc(size);
	if (!table) err(EX_OSERR, "malloc");
	memset(table, 0, size);
	return table;
}

static struct TableIter {
	struct Table *table;
	size_t i;
	struct Vec vec;
	struct Span span;
} TableIter(struct Table *table) {
	return (struct TableIter) { table, 0, Vec(NULL, 0), Span(0, 0) };
}
static bool tableNext(struct TableIter *it) {
	if (it->i == it->table->len) return false;
	it->vec = it->table->vec[it->i++];
	it->span = Span(it->span.to, it->span.to + it->vec.len);
	return true;
}
static bool tablePrev(struct TableIter *it) {
	if (!it->i) return false;
	it->vec = it->table->vec[--it->i];
	it->span = Span(it->span.at - it->vec.len, it->span.at);
	return true;
}

static struct Table *tableInsert(struct Table *prev, size_t at, struct Vec vec) {
	struct Table *next = Table(prev->len + 2);
	if (!prev->len) {
		next->vec[next->len++] = vec;
		return next;
	}
	struct Span span = Span(at, at + vec.len);
	for (struct TableIter it = TableIter(prev); tableNext(&it); (void)it) {
		if (it.span.at == at) {
			next->vec[next->len++] = vec;
			next->vec[next->len++] = it.vec;
		} else if (spanHeadIn(span, it.span)) {
			next->vec[next->len++] = vecHead(it.vec, it.span, at);
			next->vec[next->len++] = vec;
			next->vec[next->len++] = vecTail(it.vec, it.span, at);
		} else {
			next->vec[next->len++] = it.vec;
		}
	}
	return next;
}

static struct Table *tableDelete(struct Table *prev, struct Span span) {
	struct Table *next = Table(prev->len + 1);
	for (struct TableIter it = TableIter(prev); tableNext(&it); (void)it) {
		if (spanIn(it.span, span) || spanEqual(it.span, span)) {
			// drop
		} else if (spanIn(span, it.span)) {
			next->vec[next->len++] = vecHead(it.vec, it.span, span.at);
			next->vec[next->len++] = vecTail(it.vec, it.span, span.to);
		} else if (spanHeadIn(span, it.span)) {
			next->vec[next->len++] = vecHead(it.vec, it.span, span.at);
		} else if (spanTailIn(span, it.span)) {
			next->vec[next->len++] = vecTail(it.vec, it.span, span.to);
		} else {
			next->vec[next->len++] = it.vec;
		}
	}
	return next;
}

static struct VecIter {
	struct Vec vec;
	size_t i;
	wchar_t ch;
} VecIter(struct Vec vec, size_t i) {
	return (struct VecIter) { vec, i, 0 };
}
static bool vecNext(struct VecIter *it) {
	if (it->i >= it->vec.len) return false;
	int len = mbtowc(&it->ch, &it->vec.ptr[it->i], it->vec.len - it->i);
	if (len < 0) errx(EX_DATAERR, "mbtowc");
	it->i += len;
	return true;
}
static bool vecPrev(struct VecIter *it) {
	if (!it->i) return false;
	int len;
	for (int n = 1; n < MB_CUR_MAX; ++n) {
		len = mbtowc(&it->ch, &it->vec.ptr[it->i - n], n);
		if (len > 0) break;
	}
	if (len < 0) errx(EX_DATAERR, "mbtowc");
	it->i -= len;
	return true;
}

//static void curse(void) {
//	setlocale(LC_CTYPE, "");
//	initscr();
//	cbreak();
//	noecho();
//	keypad(stdscr, true);
//	set_escdelay(100);
//}
//
//static void curseChar(wchar_t ch, attr_t attr, short color) {
//	cchar_t cc;
//	wchar_t ws[] = { ch, 0 };
//	setcchar(&cc, ws, attr, color, NULL);
//	add_wch(&cc);
//}

static void draw(struct Table *table) {
	//move(0, 0);
	struct TableIter it = TableIter(table);
	tableNext(&it);
	printf("(%zu,%zu)%.*s", it.span.at, it.span.to, (int)it.vec.len, it.vec.ptr);
	tableNext(&it);
	printf("(%zu,%zu)%.*s", it.span.at, it.span.to, (int)it.vec.len, it.vec.ptr);
	tablePrev(&it);
	printf("(%zu,%zu)%.*s", it.span.at, it.span.to, (int)it.vec.len, it.vec.ptr);
	tablePrev(&it);
	printf("(%zu,%zu)%.*s", it.span.at, it.span.to, (int)it.vec.len, it.vec.ptr);
}

int main() {
	struct Table *table = Table(0);
	table = tableInsert(table, 0, Vec("Hello, world!\nGoodbye, world!\n", 30));
	table = tableDelete(table, Span(1, 5));
	table = tableInsert(table, 1, Vec("owdy", 4));
	table = tableDelete(table, Span(3, 6));
	table = tableInsert(table, 3, Vec(" Ω", 3));

	//curse();
	draw(table);
	//getch();
	//endwin();
}
