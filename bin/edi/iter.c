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

#include <stdlib.h>
#include <wchar.h>

#include "edi.h"

struct Iter iter(const struct Table *table, size_t at) {
	struct Span span = { 0, 0 };
	size_t slice;
	for (slice = 0; slice < table->len; ++slice) {
		span = spanNext(span, table->slices[slice].len);
		if (span.at <= at && span.to > at) break;
	}
	return (struct Iter) {
		.table = table,
		.span = span,
		.slice = slice,
		.at = (at < span.to ? at : span.to),
		.ch = (at < span.to ? table->slices[slice].ptr[at - span.at] : WEOF),
	};
}

struct Iter iterNext(struct Iter it) {
	if (it.at == it.span.to && it.ch == WEOF) return it;
	it.at++;
	if (it.at == it.span.to) {
		if (it.slice + 1 == it.table->len) {
			it.ch = WEOF;
			return it;
		}
		it.slice++;
		it.span = spanNext(it.span, it.table->slices[it.slice].len);
	}
	it.ch = it.table->slices[it.slice].ptr[it.at - it.span.at];
	return it;
}

struct Iter iterPrev(struct Iter it) {
	if (it.at > it.span.to && it.ch == WEOF) return it;
	it.at--;
	if (it.at > it.span.to) {
		it.ch = WEOF;
		return it;
	}
	if (it.at < it.span.at) {
		it.slice--;
		it.span = spanPrev(it.span, it.table->slices[it.slice].len);
	}
	it.ch = it.table->slices[it.slice].ptr[it.at - it.span.at];
	return it;
}

#ifdef TEST
#include <assert.h>

int main() {
	struct Slice slices[2] = {
		{ L"AB", 2 },
		{ L"CD", 2 },
	};
	struct Table table = { .len = 2, .slices = slices };

	assert(L'A' == iter(&table, 0).ch);
	assert(L'B' == iter(&table, 1).ch);
	assert(L'C' == iter(&table, 2).ch);
	assert(L'D' == iter(&table, 3).ch);
	assert(WEOF == iter(&table, 4).ch);

	assert(L'B' == iterNext(iter(&table, 0)).ch);
	assert(L'C' == iterNext(iter(&table, 1)).ch);
	assert(L'D' == iterNext(iter(&table, 2)).ch);
	assert(WEOF == iterNext(iter(&table, 5)).ch);

	assert(WEOF == iterPrev(iter(&table, 0)).ch);
	assert(L'A' == iterPrev(iter(&table, 1)).ch);
	assert(L'B' == iterPrev(iter(&table, 2)).ch);
	assert(L'C' == iterPrev(iter(&table, 3)).ch);

	struct Iter it = iter(&table, 3);
	it = iterNext(it);
	it = iterNext(it);
	assert(WEOF == it.ch);
	it = iterPrev(it);
	assert(L'D' == it.ch);

	it = iter(&table, 0);
	it = iterPrev(it);
	it = iterPrev(it);
	assert(WEOF == it.ch);
	it = iterNext(it);
	assert(L'A' == it.ch);
}

#endif
