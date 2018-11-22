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

struct Table tableAlloc(size_t cap) {
	struct Slice *slices = malloc(sizeof(*slices) * cap);
	if (!slices) err(EX_OSERR, "malloc");
	return (struct Table) {
		.cap = cap,
		.len = 0,
		.ins = 0,
		.slices = slices,
	};
}

void tableFree(struct Table *table) {
	free(table->slices);
}

void tablePush(struct Table *table, struct Slice slice) {
	if (table->len == table->cap) {
		table->cap *= 2;
		table->slices = realloc(
			table->slices,
			sizeof(*table->slices) * table->cap
		);
		if (!table->slices) err(EX_OSERR, "malloc");
	}
	table->slices[table->len++] = slice;
}

struct Table tableInsert(const struct Table *prev, size_t at, struct Slice ins) {
	struct Table next = tableAlloc(prev->len + 2);
	struct Span span = { 0, 0 };
	for (size_t i = 0; i < prev->len; ++i) {
		span = spanNext(span, prev->slices[i].len);
		if (span.at == at) {
			next.slices[next.ins = next.len++] = ins;
			next.slices[next.len++] = prev->slices[i];
		} else if (span.at < at && span.to > at) {
			next.slices[next.len++] = (struct Slice) {
				prev->slices[i].ptr,
				at - span.at,
			};
			next.slices[next.ins = next.len++] = ins;
			next.slices[next.len++] = (struct Slice) {
				&prev->slices[i].ptr[at - span.at],
				prev->slices[i].len - (at - span.at),
			};
		} else {
			next.slices[next.len++] = prev->slices[i];
		}
	}
	if (span.to == at) {
		next.slices[next.ins = next.len++] = ins;
	}
	return next;
}

void tableUpdate(struct Table *table, struct Slice ins) {
	if (table->ins < table->len) table->slices[table->ins] = ins;
}

struct Table tableDelete(const struct Table *prev, struct Span del) {
	struct Table next = tableAlloc(prev->len + 1);
	struct Span span = { 0, 0 };
	for (size_t i = 0; i < prev->len; ++i) {
		span = spanNext(span, prev->slices[i].len);
		if (span.at >= del.at && span.to <= del.to) {
			(void)prev->slices[i];
		} else if (span.at < del.at && span.to > del.to) {
			next.slices[next.len++] = (struct Slice) {
				prev->slices[i].ptr,
				del.at - span.at,
			};
			next.slices[next.len++] = (struct Slice) {
				&prev->slices[i].ptr[del.to - span.at],
				prev->slices[i].len - (del.to - span.at),
			};
		} else if (span.at < del.at && span.to > del.at) {
			next.slices[next.len++] = (struct Slice) {
				prev->slices[i].ptr,
				del.at - span.at,
			};
		} else if (span.at < del.to && span.to > del.to) {
			next.slices[next.len++] = (struct Slice) {
				&prev->slices[i].ptr[del.to - span.at],
				prev->slices[i].len - (del.to - span.at),
			};
		} else {
			next.slices[next.len++] = prev->slices[i];
		}
	}
	next.ins = next.len;
	return next;
}

#ifdef TEST
#include <assert.h>

static struct Slice slice(const wchar_t *str) {
	return (struct Slice) { str, wcslen(str) };
}

static struct Span span(size_t at, size_t to) {
	return (struct Span) { at, to };
}

static int eq(const wchar_t *str, const struct Table *table) {
	for (size_t i = 0; i < table->len; ++i) {
		if (wcsncmp(str, table->slices[i].ptr, table->slices[i].len)) {
			return 0;
		}
		str = &str[table->slices[i].len];
	}
	return !str[0];
}

int main() {
	struct Table abc = tableInsert(&TableEmpty, 0, slice(L"ABC"));
	assert(eq(L"ABC", &abc));

	struct Table dabc = tableInsert(&abc, 0, slice(L"D"));
	struct Table abcd = tableInsert(&abc, 3, slice(L"D"));
	struct Table adbc = tableInsert(&abc, 1, slice(L"D"));

	assert(eq(L"DABC", &dabc));
	assert(eq(L"ABCD", &abcd));
	assert(eq(L"ADBC", &adbc));

	assert(L'D' == dabc.slices[dabc.ins].ptr[0]);
	assert(L'D' == abcd.slices[abcd.ins].ptr[0]);
	assert(L'D' == adbc.slices[adbc.ins].ptr[0]);

	tableFree(&dabc);
	tableFree(&abcd);
	tableFree(&adbc);

	struct Table bc = tableDelete(&abc, span(0, 1));
	struct Table ab = tableDelete(&abc, span(2, 3));
	struct Table ac = tableDelete(&abc, span(1, 2));
	struct Table __ = tableDelete(&abc, span(0, 3));

	assert(eq(L"BC", &bc));
	assert(eq(L"AB", &ab));
	assert(eq(L"AC", &ac));
	assert(eq(L"",   &__));

	tableFree(&bc);
	tableFree(&ab);
	tableFree(&ac);
	tableFree(&__);

	tableFree(&abc);
}

#endif
