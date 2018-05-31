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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <wchar.h>

static struct Span {
	size_t at;
	size_t to;
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

static struct Seg {
	char *ptr;
	size_t len;
} Seg(char *ptr, size_t len) {
	return (struct Seg) { ptr, len };
}

static struct Seg segHead(struct Seg seg, size_t at) {
	return Seg(seg.ptr, at);
}
static struct Seg segTail(struct Seg seg, size_t at) {
	return Seg(seg.ptr + at, seg.len - at);
}

static struct Table {
	struct Table *next;
	struct Table *prev;
	size_t len;
	struct Seg seg[];
} *Table(size_t cap) {
	size_t size = sizeof(struct Table) + cap * sizeof(struct Seg);
	struct Table *table = malloc(size);
	if (!table) err(EX_OSERR, "malloc");
	memset(table, 0, size);
	return table;
}

static struct TableIter {
	size_t len;
	struct Seg *seg;
	struct Span span;
} TableIter(struct Table *table) {
	if (table->len) {
		return (struct TableIter) {
			table->len,
			&table->seg[0],
			Span(0, table->seg[0].len),
		};
	} else {
		return (struct TableIter) { 0, NULL, Span(0, 0) };
	}
}

static void tableNext(struct TableIter *it) {
	it->len--;
	it->seg++;
	it->span = Span(it->span.to, it->span.to + it->seg->len);
}

static struct Seg *tableAt(struct Table *table, size_t at) {
	for (struct TableIter it = TableIter(table); it.len; tableNext(&it)) {
		if (it.span.at == at) return it.seg;
	}
	return NULL;
}

static struct Table *tableInsert(struct Table *prev, size_t at, struct Seg seg) {
	struct Table *next = Table(prev->len + 2);
	if (!prev->len) {
		next->seg[next->len++] = seg;
		return next;
	}
	struct Span span = Span(at, at + seg.len);
	for (struct TableIter it = TableIter(prev); it.len; tableNext(&it)) {
		if (it.span.at == at) {
			next->seg[next->len++] = seg;
			next->seg[next->len++] = *it.seg;
		} else if (spanHeadIn(span, it.span)) {
			next->seg[next->len++] = segHead(*it.seg, at - it.span.at);
			next->seg[next->len++] = seg;
			next->seg[next->len++] = segTail(*it.seg, at - it.span.at);
		} else {
			next->seg[next->len++] = *it.seg;
		}
	}
	return next;
}

static struct Table *tableDelete(struct Table *prev, struct Span span) {
	struct Table *next = Table(prev->len + 1);
	for (struct TableIter it = TableIter(prev); it.len; tableNext(&it)) {
		if (spanIn(it.span, span) || spanEqual(it.span, span)) {
			// drop
		} else if (spanIn(span, it.span)) {
			next->seg[next->len++] = segHead(*it.seg, span.at - it.span.at);
			next->seg[next->len++] = segTail(*it.seg, span.to - it.span.at);
		} else if (spanHeadIn(span, it.span)) {
			next->seg[next->len++] = segHead(*it.seg, span.at - it.span.at);
		} else if (spanTailIn(span, it.span)) {
			next->seg[next->len++] = segTail(*it.seg, span.to - it.span.at);
		} else {
			next->seg[next->len++] = *it.seg;
		}
	}
	return next;
}

static struct CharIter {
	struct TableIter in;
	struct Seg seg;
	size_t at;
	wchar_t ch;
	int len;
} CharIter(struct Table *table, size_t at) {
	struct TableIter in;
	for (in = TableIter(table); in.len; tableNext(&in)) {
		if (at >= in.span.at && at < in.span.to) break;
	}
	struct CharIter it = { in, segTail(*in.seg, at - in.span.at), at, 0, 0 };
	it.len = mbtowc(&it.ch, it.seg.ptr, it.seg.len);
	if (it.len < 0) err(EX_DATAERR, "mbtowc");
	return it;
}

static void charNext(struct CharIter *it) {
	it->seg.ptr += it->len;
	it->seg.len -= it->len;
	it->at += it->len;
	if (!it->seg.len && it->in.len) {
		tableNext(&it->in);
		it->seg = *it->in.seg;
		it->at = it->in.span.at;
	}
	it->len = mbtowc(&it->ch, it->seg.ptr, it->seg.len);
	if (it->len < 0) err(EX_DATAERR, "mbtowc");
}

static void curse(void) {
	setlocale(LC_CTYPE, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, true);
	set_escdelay(100);
}

static void curseChar(wchar_t ch, attr_t attr, short color) {
	cchar_t cc;
	wchar_t ws[] = { ch, 0 };
	setcchar(&cc, ws, attr, color, NULL);
	add_wch(&cc);
}

static void draw(struct Table *table, size_t at, struct Span cursor) {
	move(0, 0);
	int y, x;
	for (struct CharIter it = CharIter(table, at); it.seg.len; charNext(&it)) {
		if (it.at == cursor.at) getyx(stdscr, y, x);
		attr_t attr = A_NORMAL;
		if (it.at >= cursor.at && it.at < cursor.to) attr = A_REVERSE;
		curseChar(it.ch, attr, 0);
	}
	move(y, x);
}

int main() {
	struct Table *table = Table(0);
	table = tableInsert(table, 0, Seg("Hello, world!\nGoodbye, world!\n", 30));
	table = tableDelete(table, Span(1, 5));
	table = tableInsert(table, 1, Seg("owdy", 4));
	table = tableDelete(table, Span(3, 6));
	table = tableInsert(table, 3, Seg(" do", 3));

	curse();
	draw(table, 0, Span(1, 3));
	getch();
	endwin();
}
