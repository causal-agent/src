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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "edi.h"

static const char Magic[3] = "EDI";
static const char Version  = 1;

static enum Error
write(FILE *stream, const void *ptr, size_t size, size_t nitems) {
	size_t n = fwrite(ptr, size, nitems, stream);
	return (n < nitems ? Errno + errno : Ok);
}

static enum Error writeBuffer(FILE *stream, const struct Buffer *buf) {
	size_t len = 0;
	const struct Block *block;
	for (block = buf->block; block; block = block->prev) {
		len += block->len;
	}
	enum Error error = write(stream, &len, sizeof(len), 1);
	if (error) return error;
	for (block = buf->block; block; block = block->prev) {
		error = write(stream, block->chars, sizeof(wchar_t), block->len);
		if (error) return error;
	}
	return Ok;
}

static enum Error
writeSlice(FILE *stream, const struct Buffer *buf, struct Slice slice) {
	size_t offset = 0;
	const struct Block *block = buf->block;
	while (block) {
		if (
			slice.ptr >= block->chars && slice.ptr < &block->chars[block->len]
		) break;
		offset += block->len;
		block = block->prev;
	}
	assert(block);
	size_t ptr = offset + (size_t)(slice.ptr - block->chars);
	enum Error error = write(stream, &ptr, sizeof(ptr), 1);
	if (error) return error;
	return write(stream, &slice.len, sizeof(slice.len), 1);
}

static enum Error
writeTable(FILE *stream, const struct Buffer *buf, const struct Table *table) {
	enum Error error = write(stream, &table->len, sizeof(table->len), 1);
	for (size_t i = 0; i < table->len; ++i) {
		error = writeSlice(stream, buf, table->slices[i]);
		if (error) return error;
	}
	return Ok;
}

static enum Error
writeState(FILE *stream, const struct Buffer *buf, const struct State *state) {
	enum Error error;
	error = write(stream, &state->prev, sizeof(state->prev), 1);
	if (error) return error;
	error = write(stream, &state->next, sizeof(state->next), 1);
	if (error) return error;
	return writeTable(stream, buf, &state->table);
}

static enum Error
writeLog(FILE *stream, const struct Buffer *buf, const struct Log *log) {
	enum Error error;
	error = write(stream, &log->state, sizeof(log->state), 1);
	if (error) return error;
	error = write(stream, &log->len, sizeof(log->len), 1);
	if (error) return error;
	for (size_t i = 0; i < log->len; ++i) {
		error = writeState(stream, buf, &log->states[i]);
		if (error) return error;
	}
	return Ok;
}

enum Error storeWrite(FILE *stream, const struct Edit *edit) {
	enum Error error;
	error = write(stream, Magic, sizeof(Magic), 1);
	if (error) return error;
	error = write(stream, &Version, sizeof(Version), 1);
	if (error) return error;
	error = writeBuffer(stream, &edit->buf);
	if (error) return error;
	return writeLog(stream, &edit->buf, &edit->log);
}

static enum Error read(FILE *stream, void *ptr, size_t size, size_t nitems) {
	size_t n = fread(ptr, size, nitems, stream);
	if (ferror(stream)) return Errno + errno;
	return (n < nitems ? StoreEOF : Ok);
}

static enum Error readMagic(FILE *stream) {
	char magic[3];
	enum Error error = read(stream, magic, sizeof(magic), 1);
	if (error) return error;
	return (
		magic[0] != Magic[0] || magic[1] != Magic[1] || magic[2] != Magic[2]
		? StoreMagic
		: Ok
	);
}

static enum Error readVersion(FILE *stream) {
	char version;
	enum Error error = read(stream, &version, sizeof(version), 1);
	if (error) return error;
	return (version != Version ? StoreVersion : Ok);
}

static enum Error readBuffer(FILE *stream, struct Buffer *buf) {
	size_t len;
	enum Error error = read(stream, &len, sizeof(len), 1);
	if (error) return error;
	wchar_t *dest = bufferDest(buf, len);
	return read(stream, dest, sizeof(wchar_t), len);
}

static enum Error
readSlice(FILE *stream, struct Buffer *buf, struct Table *table) {
	enum Error error;
	size_t ptr, len;
	error = read(stream, &ptr, sizeof(ptr), 1);
	if (error) return error;
	error = read(stream, &len, sizeof(len), 1);
	if (error) return error;
	tablePush(table, (struct Slice) { &buf->slice.ptr[ptr], len });
	return Ok;
}

static enum Error
readTable(FILE *stream, struct Buffer *buf, struct Table *table) {
	size_t len;
	enum Error error = read(stream, &len, sizeof(len), 1);
	if (error) return error;
	*table = tableAlloc(len);
	for (size_t i = 0; i < len; ++i) {
		error = readSlice(stream, buf, table);
		if (error) return error;
	}
	return Ok;
}

static enum Error
readState(FILE *stream, struct Buffer *buf, size_t offset, struct Log *log) {
	enum Error error;
	size_t prev, next;
	struct Table table;
	error = read(stream, &prev, sizeof(prev), 1);
	if (error) return error;
	error = read(stream, &next, sizeof(next), 1);
	if (error) return error;
	error = readTable(stream, buf, &table);
	if (error) return error;
	logPush(log, table);
	log->states[log->state].prev = offset + prev;
	log->states[log->state].next = offset + next;
	return Ok;
}

static enum Error readLog(FILE *stream, struct Buffer *buf, struct Log *log) {
	enum Error error;
	size_t state, len;
	error = read(stream, &state, sizeof(state), 1);
	if (error) return error;
	error = read(stream, &len, sizeof(len), 1);
	if (error) return error;
	size_t offset = log->len;
	for (size_t i = 0; i < len; ++i) {
		error = readState(stream, buf, offset, log);
		if (error) return error;
	}
	log->state = offset + state;
	return Ok;
}

enum Error storeRead(FILE *stream, struct Edit *edit) {
	enum Error error;
	error = readMagic(stream);
	if (error) return error;
	error = readVersion(stream);
	if (error) return error;
	error = readBuffer(stream, &edit->buf);
	if (error) return error;
	return readLog(stream, &edit->buf, &edit->log);
}
