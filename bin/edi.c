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

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

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

int main() {
	struct Table *table = Table(0);
	table = tableInsert(table, 0, Seg("Hello, world!\n", 14));
	table = tableDelete(table, Span(1, 5));
	table = tableInsert(table, 1, Seg("owdy", 5));
	table = tableDelete(table, Span(3, 6));
	table = tableInsert(table, 3, Seg(" do", 3));

	for (size_t i = 0; i < table->len; ++i) {
		printf("%.*s", (int)table->seg[i].len, table->seg[i].ptr);
	}
}
