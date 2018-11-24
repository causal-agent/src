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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "edi.h"

static const char Magic[3] = "EDI";
static const char Version  = 1;

static bool writeBuffer(FILE *stream, const struct Buffer *buf) {
	size_t len = 0;
	const struct Block *block;
	for (block = buf->block; block; block = block->prev) {
		len += block->len;
	}
	if (!fwrite(&len, sizeof(len), 1, stream)) return false;
	for (block = buf->block; block; block = block->prev) {
		fwrite(block->chars, sizeof(wchar_t), block->len, stream);
		if (ferror(stream)) return false;
	}
	return true;
}

static bool writeSlice(
	FILE *stream, const struct Buffer *buf, struct Slice slice
) {
	size_t offset = 0;
	const struct Block *block;
	for (block = buf->block; block; offset += block->len, block = block->prev) {
		if (
			slice.ptr >= block->chars && slice.ptr < &block->chars[block->len]
		) break;
	}
	assert(block);
	size_t ptr = offset + (size_t)(slice.ptr - block->chars);
	return fwrite(&ptr, sizeof(ptr), 1, stream)
		&& fwrite(&slice.len, sizeof(slice.len), 1, stream);
}

static bool writeTable(
	FILE *stream, const struct Buffer *buf, const struct Table *table
) {
	if (!fwrite(&table->len, sizeof(table->len), 1, stream)) return false;
	for (size_t i = 0; i < table->len; ++i) {
		if (!writeSlice(stream, buf, table->slices[i])) return false;
	}
	return true;
}

static bool writeState(
	FILE *stream, const struct Buffer *buf, const struct State *state
) {
	return fwrite(&state->prev, sizeof(state->prev), 1, stream)
		&& fwrite(&state->next, sizeof(state->next), 1, stream)
		&& writeTable(stream, buf, &state->table);
}

static bool writeLog(
	FILE *stream, const struct Buffer *buf, const struct Log *log
) {
	if (!fwrite(&log->state, sizeof(log->state), 1, stream)) return false;
	if (!fwrite(&log->len, sizeof(log->len), 1, stream)) return false;
	for (size_t i = 0; i < log->len; ++i) {
		if (!writeState(stream, buf, &log->states[i])) return false;
	}
	return true;
}

bool storeWrite(FILE *stream, const struct Edit *edit) {
	return fwrite(Magic, sizeof(Magic), 1, stream)
		&& fwrite(&Version, sizeof(Version), 1, stream)
		&& writeBuffer(stream, &edit->buf)
		&& writeLog(stream, &edit->buf, &edit->log);
}
