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

static bool spanStartsIn(struct Span a, struct Span b) {
	return a.at >= b.at && a.at < b.to;
}
static bool spanEndsIn(struct Span a, struct Span b) {
	return a.to > b.at && a.to <= b.to;
}
static bool spanContains(struct Span a, struct Span b) {
	return a.at <= b.at && a.to >= b.to;
}

static struct Seg {
	struct Span span;
	char *data;
} Seg(struct Span span, char *data) {
	return (struct Seg) { span, data };
}

static struct Seg segBefore(struct Seg seg, size_t at) {
	return Seg(Span(seg.span.at, at), seg.data);
}
static struct Seg segAfter(struct Seg seg, size_t at) {
	return Seg(Span(at, seg.span.to), seg.data + (at - seg.span.at));
}

struct Table {
	struct Table *next;
	struct Table *prev;
	size_t len;
	struct Seg seg[];
};

static struct Table *tableNew(size_t cap) {
	size_t size = sizeof(struct Table) + cap * sizeof(struct Seg);
	struct Table *table = malloc(size);
	if (!table) err(EX_OSERR, "malloc");
	memset(table, 0, size);
	return table;
}

static void tableAdd(struct Table *table, struct Seg seg) {
	// FIXME: Make this clearer.
	size_t len = seg.span.to - seg.span.at;
	if (table->len) {
		struct Seg last = table->seg[table->len - 1];
		seg.span = Span(last.span.to, last.span.to + len);
	} else {
		seg.span = Span(0, len);
	}
	table->seg[table->len++] = seg;
}

static struct Table *tableInsert(struct Table *prev, struct Seg seg) {
	struct Table *next = tableNew(prev->len + 2);
	if (!prev->len) {
		tableAdd(next, seg);
		return next;
	}
	for (size_t i = 0; i < prev->len; ++i) {
		if (seg.span.at == prev->seg[i].span.at) {
			tableAdd(next, seg);
			tableAdd(next, prev->seg[i]);
		} else if (spanStartsIn(seg.span, prev->seg[i].span)) {
			tableAdd(next, segBefore(prev->seg[i], seg.span.at));
			tableAdd(next, seg);
			tableAdd(next, segAfter(prev->seg[i], seg.span.at));
		} else {
			tableAdd(next, prev->seg[i]);
		}
	}
	return next;
}

static struct Table *tableDelete(struct Table *prev, struct Span span) {
	struct Table *next = tableNew(prev->len + 1);
	for (size_t i = 0; i < prev->len; ++i) {
		if (spanContains(span, prev->seg[i].span)) {
			// drop
		} else if (spanContains(prev->seg[i].span, span)) {
			tableAdd(next, segBefore(prev->seg[i], span.at));
			tableAdd(next, segAfter(prev->seg[i], span.to));
		} else if (spanStartsIn(span, prev->seg[i].span)) {
			tableAdd(next, segBefore(prev->seg[i], span.at));
		} else if (spanEndsIn(span, prev->seg[i].span)) {
			tableAdd(next, segAfter(prev->seg[i], span.to));
		} else {
			tableAdd(next, prev->seg[i]);
		}
	}
	return next;
}

int main() {
	struct Table *table = tableNew(0);
	table = tableInsert(table, Seg(Span(0, 13), "Hello, world!"));
	table = tableDelete(table, Span(1, 5));
	table = tableInsert(table, Seg(Span(1, 5), "owdy"));
	table = tableDelete(table, Span(3, 6));
	table = tableInsert(table, Seg(Span(3, 6), " do"));

	for (size_t i = 0; i < table->len; ++i) {
		printf("%.*s", table->seg[i].span.to - table->seg[i].span.at, table->seg[i].data);
	}
}
