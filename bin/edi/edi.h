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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

struct Span {
	size_t at, to;
};
static inline struct Span spanNext(struct Span span, size_t len) {
	return (struct Span) { span.to, span.to + len };
}
static inline struct Span spanPrev(struct Span span, size_t len) {
	return (struct Span) { span.at - len, span.at };
}

struct Slice {
	const wchar_t *ptr;
	size_t len;
};

struct Buffer {
	size_t cap;
	struct Slice slice;
	struct Block {
		struct Block *prev;
		size_t len;
		wchar_t chars[];
	} *block;
};
struct Buffer bufferAlloc(size_t cap);
void bufferFree(struct Buffer *buf);
void bufferSlice(struct Buffer *buf);
void bufferPush(struct Buffer *buf, wchar_t ch);
void bufferPop(struct Buffer *buf);
wchar_t *bufferDest(struct Buffer *buf, size_t len);
void bufferTruncate(struct Buffer *buf, size_t len);

static const struct Table {
	size_t cap, len;
	struct Slice *slices;
} TableEmpty;
struct Table tableAlloc(size_t cap);
void tableFree(struct Table *table);
void tablePush(struct Table *table, struct Slice slice);
void tableReplace(struct Table *table, struct Slice old, struct Slice new);
struct Table tableInsert(const struct Table *prev, size_t at, struct Slice ins);
struct Table tableDelete(const struct Table *prev, struct Span del);

struct Iter {
	const struct Table *table;
	struct Span span;
	size_t slice;
	size_t at;
	wint_t ch;
};
struct Iter iter(const struct Table *table, size_t at);
struct Iter iterNext(struct Iter it);
struct Iter iterPrev(struct Iter it);

struct Log {
	size_t cap, len;
	size_t state;
	struct State {
		size_t prev, next;
		struct Table table;
	} *states;
};
struct Log logAlloc(size_t cap);
void logFree(struct Log *log);
void logPush(struct Log *log, struct Table table);
static inline struct Table *logTable(const struct Log *log) {
	if (log->state == log->len) return NULL;
	return &log->states[log->state].table;
}

struct Edit {
	struct Buffer buf;
	struct Log log;
};

bool storeWrite(FILE *stream, const struct Edit *edit);
bool storeRead(FILE *stream, struct Edit *edit);

struct File {
	char *path;
	struct Buffer buf;
	struct Log log;
	size_t clean;
};
struct File fileAlloc(char *path);
void fileFree(struct File *file);
void fileRead(struct File *file);
void fileWrite(struct File *file);
