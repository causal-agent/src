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
	block->len = 0;
	return block;
}

struct Buffer bufferAlloc(size_t cap) {
	return (struct Buffer) {
		.cap = cap,
		.block = blockAlloc(NULL, cap),
	};
}

void bufferFree(struct Buffer *buf) {
	struct Block *prev;
	for (struct Block *it = buf->block; it; it = prev) {
		prev = it->prev;
		free(it);
	}
}

void bufferSlice(struct Buffer *buf) {
	buf->slice.ptr = &buf->block->chars[buf->block->len];
	buf->slice.len = 0;
}

void bufferPush(struct Buffer *buf, wchar_t ch) {
	if (buf->block->len == buf->cap) {
		if (buf->slice.len == buf->cap) buf->cap *= 2;
		buf->block = blockAlloc(buf->block, buf->cap);
		memcpy(
			buf->block->chars, buf->slice.ptr,
			sizeof(wchar_t) * buf->slice.len
		);
		buf->slice.ptr = buf->block->chars;
		buf->block->len = buf->slice.len;
	}
	buf->block->chars[buf->block->len++] = ch;
	buf->slice.len++;
}

void bufferPop(struct Buffer *buf) {
	if (!buf->slice.len) return;
	buf->slice.len--;
	buf->block->len--;
}

wchar_t *bufferDest(struct Buffer *buf, size_t len) {
	if (buf->block->len + len > buf->cap) {
		while (len > buf->cap) buf->cap *= 2;
		buf->block = blockAlloc(buf->block, buf->cap);
	}
	wchar_t *ptr = &buf->block->chars[buf->block->len];
	buf->slice.ptr = ptr;
	buf->slice.len = len;
	buf->block->len += len;
	return ptr;
}

void bufferTruncate(struct Buffer *buf, size_t len) {
	if (len > buf->slice.len) return;
	buf->block->len -= buf->slice.len - len;
	buf->slice.len = len;
}

#ifdef TEST
#include <assert.h>

int main() {
	struct Buffer buf = bufferAlloc(6);

	bufferSlice(&buf);
	bufferPush(&buf, L'A');
	bufferPush(&buf, L'B');
	assert(!wcsncmp(L"AB", buf.slice.ptr, buf.slice.len));

	bufferSlice(&buf);
	bufferPush(&buf, L'C');
	bufferPush(&buf, L'D');
	assert(!wcsncmp(L"CD", buf.slice.ptr, buf.slice.len));

	bufferSlice(&buf);
	bufferPush(&buf, L'E');
	bufferPush(&buf, L'F');
	bufferPush(&buf, L'G');
	bufferPush(&buf, L'H');
	assert(!wcsncmp(L"EFGH", buf.slice.ptr, buf.slice.len));

	bufferFree(&buf);

	buf = bufferAlloc(4);
	bufferSlice(&buf);
	bufferPush(&buf, L'A');
	bufferPush(&buf, L'B');
	bufferPush(&buf, L'C');
	bufferPush(&buf, L'D');
	bufferPush(&buf, L'E');
	bufferPush(&buf, L'F');
	assert(!wcsncmp(L"ABCDEF", buf.slice.ptr, buf.slice.len));
	bufferFree(&buf);

	buf = bufferAlloc(4);
	bufferSlice(&buf);
	bufferPush(&buf, L'A');
	bufferPush(&buf, L'B');
	bufferPop(&buf);
	assert(!wcsncmp(L"A", buf.slice.ptr, buf.slice.len));
	bufferPush(&buf, L'C');
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
	bufferTruncate(&buf, 4);
	assert(!wcsncmp(L"ABCD", buf.slice.ptr, buf.slice.len));
	bufferFree(&buf);
}

#endif
