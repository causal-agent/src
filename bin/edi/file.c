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

enum {
	BufferCap = 8192,
	TableCap = 2,
	LogCap = 8,
};

struct File fileAlloc(char *path) {
	struct File file = {
		.path = path,
		.edit = {
			.buf = bufferAlloc(BufferCap),
			.log = logAlloc(LogCap),
		},
	};
	if (!path) logPush(&file.edit.log, TableEmpty);
	return file;
}

void fileFree(struct File *file) {
	logFree(&file->edit.log);
	bufferFree(&file->edit.buf);
	free(file->path);
}

static const mbstate_t StateInit;

enum Error fileRead(struct File *file) {
	if (!file->path) return FileNoPath;

	FILE *stream = fopen(file->path, "r");
	if (!stream) {
		if (errno != ENOENT) return Errno + errno;
		logPush(&file->edit.log, TableEmpty);
		file->clean = file->edit.log.state;
		return Ok;
	}

	struct Table table = tableAlloc(TableCap);
	char buf[BufferCap];
	mbstate_t state = StateInit;
	while (!feof(stream)) {
		size_t mbsLen = fread(buf, 1, sizeof(buf), stream);
		if (ferror(stream)) return Errno + errno;

		// FIXME: Handle null bytes specially.
		const char *mbs = buf;
		wchar_t *wcs = bufferDest(&file->edit.buf, mbsLen);
		size_t wcsLen = mbsnrtowcs(wcs, &mbs, mbsLen, mbsLen, &state);
		if (wcsLen == (size_t)-1) return Errno + errno;

		bufferTruncate(&file->edit.buf, wcsLen);
		tablePush(&table, file->edit.buf.slice);
	}
	logPush(&file->edit.log, table);
	file->clean = file->edit.log.state;

	fclose(stream);
	return Ok;
}

enum Error fileWrite(struct File *file) {
	if (!file->path) return FileNoPath;

	FILE *stream = fopen(file->path, "w");
	if (!stream) return Errno + errno;

	const struct Table *table = logTable(&file->edit.log);
	assert(table);

	char buf[BufferCap];
	mbstate_t state = StateInit;
	for (size_t i = 0; i < table->len; ++i) {
		struct Slice slice = table->slices[i];
		while (slice.len) {
			// FIXME: Handle null bytes specially.
			size_t mbsLen = wcsnrtombs(
				buf, &slice.ptr, slice.len, sizeof(buf), &state
			);
			if (mbsLen == (size_t)-1) return Errno + errno;
			// FIXME: This only works once.
			slice.len -= slice.ptr - table->slices[i].ptr;

			fwrite(buf, 1, mbsLen, stream);
			if (ferror(stream)) return Errno + errno;
		}
	}
	file->clean = file->edit.log.state;

	fclose(stream);
	return (ferror(stream) ? Errno + errno : Ok);
}
