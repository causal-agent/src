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

static inline struct Span {
	size_t at, to;
} spanNext(struct Span span, size_t len) {
	return (struct Span) { span.to, span.to + len };
}

struct Slice {
	const wchar_t *ptr;
	size_t len;
};

struct Buffer {
	size_t cap, len;
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

static const struct Table {
	size_t cap, len;
	size_t ins;
	struct Slice *slices;
} TableEmpty;
struct Table tableAlloc(size_t cap);
void tableFree(struct Table *table);
void tablePush(struct Table *table, struct Slice slice);
struct Table tableInsert(const struct Table *prev, size_t at, struct Slice ins);
struct Table tableDelete(const struct Table *prev, struct Span del);
void tableUpdate(struct Table *table, struct Slice ins);

struct Log {
	size_t cap, len;
	size_t state;
	struct State {
		struct Table table;
		size_t prev, next;
	} *states;
};
struct Log logAlloc(size_t cap);
void logFree(struct Log *log);
void logPush(struct Log *log, struct Table table);

struct File {
	char *path;
	struct Buffer buf;
	struct Log log;
	size_t clean;
};
struct File fileAlloc(char *path);
void fileFree(struct File *file);
void fileRead(struct File *file);
