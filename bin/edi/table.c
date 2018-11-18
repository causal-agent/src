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
#include <stdlib.h>
#include <sysexits.h>
#include <wchar.h>

#include "edi.h"

static struct Table *alloc(size_t cap) {
	size_t size = sizeof(struct Table) + sizeof(struct Slice) * cap;
	struct Table *table = malloc(size);
	if (!table) err(EX_OSERR, "malloc");
	table->len = 0;
	return table;
}

struct Table *tableInsert(const struct Table *prev, size_t at, struct Slice slice) {
	if (!prev || !prev->len) {
		struct Table *next = alloc(1);
		next->slices[next->len++] = slice;
		return next;
	}

	struct Table *next = alloc(prev->len + 2);
	struct Span span = { 0, 0 };
	for (size_t i = 0; i < prev->len; ++i) {
		span = spanNext(span, prev->slices[i].len);
		if (span.at == at) {
			next->slices[next->len++] = slice;
			next->slices[next->len++] = prev->slices[i];
		} else if (span.at < at && span.to > at) {
			next->slices[next->len++] = (struct Slice) {
				prev->slices[i].ptr,
				at - span.at,
			};
			next->slices[next->len++] = slice;
			next->slices[next->len++] = (struct Slice) {
				&prev->slices[i].ptr[at - span.at],
				prev->slices[i].len - (at - span.at),
			};
		} else {
			next->slices[next->len++] = prev->slices[i];
		}
	}
	if (span.to == at) {
		next->slices[next->len++] = slice;
	}
	return next;
}

struct Table *tableDelete(const struct Table *prev, struct Span del) {
	if (!prev || !prev->len) return alloc(0);

	struct Table *next = alloc(prev->len + 1);
	struct Span span = { 0, 0 };
	for (size_t i = 0; i < prev->len; ++i) {
		span = spanNext(span, prev->slices[i].len);
		if (span.at >= del.at && span.to <= del.to) {
			(void)prev->slices[i];
		} else if (span.at < del.at && span.to > del.to) {
			next->slices[next->len++] = (struct Slice) {
				prev->slices[i].ptr,
				del.at - span.at,
			};
			next->slices[next->len++] = (struct Slice) {
				&prev->slices[i].ptr[del.to - span.at],
				prev->slices[i].len - (del.to - span.at),
			};
		} else if (span.at < del.at && span.to > del.at) {
			next->slices[next->len++] = (struct Slice) {
				prev->slices[i].ptr,
				del.at - span.at,
			};
		} else if (span.at < del.to && span.to > del.to) {
			next->slices[next->len++] = (struct Slice) {
				&prev->slices[i].ptr[del.to - span.at],
				prev->slices[i].len - (del.to - span.at),
			};
		} else {
			next->slices[next->len++] = prev->slices[i];
		}
	}
	return next;
}

#ifdef TEST
#include <assert.h>

static struct Span span(size_t at, size_t to) {
	return (struct Span) { at, to };
}

static struct Slice slice(const wchar_t *str) {
	return (struct Slice) { str, wcslen(str) };
}

static int eq(const struct Table *table, const wchar_t *str) {
	for (size_t i = 0; i < table->len; ++i) {
		if (wcsncmp(str, table->slices[i].ptr, table->slices[i].len)) {
			return 0;
		}
		str = &str[table->slices[i].len];
	}
	return 1;
}

int main() {
	struct Table *abc = tableInsert(NULL, 0, slice(L"ABC"));
	assert(eq(abc, L"ABC"));

	assert(eq(tableInsert(abc, 0, slice(L"D")), L"DABC"));
	assert(eq(tableInsert(abc, 3, slice(L"D")), L"ABCD"));
	assert(eq(tableInsert(abc, 1, slice(L"D")), L"ADBC"));

	assert(eq(tableDelete(abc, span(0, 1)), L"BC"));
	assert(eq(tableDelete(abc, span(2, 3)), L"AB"));
	assert(eq(tableDelete(abc, span(1, 2)), L"AC"));
	assert(eq(tableDelete(abc, span(0, 3)), L""));
}

#endif
