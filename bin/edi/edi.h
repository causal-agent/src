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

struct Span {
	size_t at, to;
};

static inline struct Span spanNext(struct Span span, size_t len) {
	return (struct Span) { span.to, span.to + len };
}

struct Slice {
	const wchar_t *ptr;
	size_t len;
};

struct Buffer {
	size_t cap;
	size_t len;
	struct Slice slice;
	struct Block {
		struct Block *prev;
		wchar_t chars[];
	} *block;
};

struct Buffer bufferAlloc(size_t cap);
void bufferFree(struct Buffer *buf);
void bufferInsert(struct Buffer *buf);
void bufferAppend(struct Buffer *buf, wchar_t ch);
void bufferDelete(struct Buffer *buf);
wchar_t *bufferDest(struct Buffer *buf, size_t len);

struct Table {
	size_t len;
	struct Slice *slices;
};
static const struct Table TableEmpty = { 0, NULL };

struct Table tableInsert(struct Table prev, size_t at, struct Slice ins);
struct Table tableDelete(struct Table prev, struct Span del);

struct Log {
	size_t cap;
	size_t len;
	size_t idx;
	struct State {
		struct Table table;
		size_t prev;
		size_t next;
	} *states;
};

struct Log logAlloc(size_t cap);
void logFree(struct Log *log);
void logPush(struct Log *log, struct Table table);

static inline struct Table logTable(struct Log log) {
	return log.states[log.idx].table;
}
static inline void logPrev(struct Log *log) {
	log->idx = log->states[log->idx].prev;
}
static inline void logNext(struct Log *log) {
	log->idx = log->states[log->idx].next;
}
static inline void logBack(struct Log *log) {
	if (log->idx) log->idx--;
}
static inline void logFore(struct Log *log) {
	if (log->idx + 1 < log->len) log->idx++;
}
