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
#include <string.h>
#include <sysexits.h>
#include <wchar.h>

#include "edi.h"

static struct Block *blockAlloc(struct Block *prev, size_t cap) {
	size_t size = sizeof(struct Block) + sizeof(wchar_t) * cap;
	struct Block *block = malloc(size);
	if (!block) err(EX_OSERR, "malloc");
	block->prev = prev;
	return block;
}

struct Buffer bufferAlloc(size_t cap) {
	struct Block *block = blockAlloc(NULL, cap);
	return (struct Buffer) {
		.cap = cap,
		.len = 0,
		.slice = { block->chars, 0 },
		.block = block,
	};
}

void bufferFree(struct Buffer *buf) {
	struct Block *prev;
	for (struct Block *it = buf->block; it; it = prev) {
		prev = it->prev;
		free(it);
	}
}

void bufferInsert(struct Buffer *buf) {
	buf->slice.ptr = &buf->block->chars[buf->len];
	buf->slice.len = 0;
}

void bufferAppend(struct Buffer *buf, wchar_t ch) {
	if (buf->len == buf->cap) {
		if (buf->slice.len == buf->cap) buf->cap *= 2;
		struct Block *block = blockAlloc(buf->block, buf->cap);
		memcpy(block->chars, buf->slice.ptr, sizeof(wchar_t) * buf->slice.len);
		buf->len = buf->slice.len;
		buf->slice.ptr = block->chars;
		buf->block = block;
	}
	buf->block->chars[buf->len++] = ch;
	buf->slice.len++;
}

void bufferDelete(struct Buffer *buf) {
	if (!buf->slice.len) return;
	buf->slice.len--;
	buf->len--;
}

wchar_t *bufferDest(struct Buffer *buf, size_t len) {
	if (buf->len + len > buf->cap) {
		while (len > buf->cap) buf->cap *= 2;
		buf->block = blockAlloc(buf->block, buf->cap);
		buf->len = 0;
	}
	wchar_t *ptr = &buf->block->chars[buf->len];
	buf->slice.ptr = ptr;
	buf->slice.len = len;
	buf->len += len;
	return ptr;
}

#ifdef TEST
#include <assert.h>

int main() {
	struct Buffer buf = bufferAlloc(6);

	bufferInsert(&buf);
	bufferAppend(&buf, L'A');
	bufferAppend(&buf, L'B');
	assert(!wcsncmp(L"AB", buf.slice.ptr, buf.slice.len));

	bufferInsert(&buf);
	bufferAppend(&buf, L'C');
	bufferAppend(&buf, L'D');
	assert(!wcsncmp(L"CD", buf.slice.ptr, buf.slice.len));

	bufferInsert(&buf);
	bufferAppend(&buf, L'E');
	bufferAppend(&buf, L'F');
	bufferAppend(&buf, L'G');
	bufferAppend(&buf, L'H');
	assert(!wcsncmp(L"EFGH", buf.slice.ptr, buf.slice.len));

	bufferFree(&buf);

	buf = bufferAlloc(4);
	bufferInsert(&buf);
	bufferAppend(&buf, L'A');
	bufferAppend(&buf, L'B');
	bufferAppend(&buf, L'C');
	bufferAppend(&buf, L'D');
	bufferAppend(&buf, L'E');
	bufferAppend(&buf, L'F');
	assert(!wcsncmp(L"ABCDEF", buf.slice.ptr, buf.slice.len));
	bufferFree(&buf);

	buf = bufferAlloc(4);
	bufferInsert(&buf);
	bufferAppend(&buf, L'A');
	bufferAppend(&buf, L'B');
	bufferDelete(&buf);
	assert(!wcsncmp(L"A", buf.slice.ptr, buf.slice.len));
	bufferAppend(&buf, L'C');
	assert(!wcsncmp(L"AC", buf.slice.ptr, buf.slice.len));
	bufferFree(&buf);

	buf = bufferAlloc(4);

	wchar_t *dest = bufferDest(&buf, 2);
	dest[0] = L'A';
	dest[1] = L'B';
	assert(!wcsncmp(L"AB", buf.slice.ptr, buf.slice.len));

	dest = bufferDest(&buf, 3);
	dest[0] = L'C';
	dest[1] = L'D';
	dest[2] = L'E';
	assert(!wcsncmp(L"CDE", buf.slice.ptr, buf.slice.len));

	bufferFree(&buf);

	buf = bufferAlloc(4);
	dest = bufferDest(&buf, 6);
	dest[0] = L'A';
	dest[1] = L'B';
	dest[2] = L'C';
	dest[3] = L'D';
	dest[4] = L'E';
	dest[5] = L'F';
	assert(!wcsncmp(L"ABCDEF", buf.slice.ptr, buf.slice.len));
	bufferFree(&buf);
}

#endif
