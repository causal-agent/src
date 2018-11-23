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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
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
		.buf = bufferAlloc(BufferCap),
		.log = logAlloc(LogCap),
	};
	if (!path) logPush(&file.log, TableEmpty);
	return file;
}

void fileFree(struct File *file) {
	logFree(&file->log);
	bufferFree(&file->buf);
	free(file->path);
}

static const mbstate_t StateInit;

// TODO: Error handling.
void fileRead(struct File *file) {
	if (!file->path) return;

	FILE *stream = fopen(file->path, "r");
	if (!stream) {
		if (errno != ENOENT) err(EX_NOINPUT, "%s", file->path);
		logPush(&file->log, TableEmpty);
		file->clean = file->log.state;
		return;
	}

	struct Table table = tableAlloc(TableCap);
	char buf[BufferCap];
	mbstate_t state = StateInit;
	while (!feof(stream)) {
		size_t mbsLen = fread(buf, 1, sizeof(buf), stream);
		if (ferror(stream)) err(EX_IOERR, "%s", file->path);

		const char *mbs = buf;
		wchar_t *wcs = bufferDest(&file->buf, mbsLen);
		size_t wcsLen = mbsnrtowcs(wcs, &mbs, mbsLen, mbsLen, &state);
		if (wcsLen == (size_t)-1) err(EX_DATAERR, "%s", file->path);

		bufferTruncate(&file->buf, wcsLen);
		tablePush(&table, file->buf.slice);
	}
	logPush(&file->log, table);
	file->clean = file->log.state;

	fclose(stream);
}

// TODO: Error handling.
void fileWrite(struct File *file) {
	if (!file->path) return;

	FILE *stream = fopen(file->path, "w");
	if (!stream) err(EX_CANTCREAT, "%s", file->path);

	const struct Table *table = logTable(&file->log);
	if (!table) errx(EX_SOFTWARE, "fileWrite: no table");

	char buf[BufferCap];
	mbstate_t state = StateInit;
	for (size_t i = 0; i < table->len; ++i) {
		struct Slice slice = table->slices[i];
		while (slice.len) {
			size_t mbsLen = wcsnrtombs(
				buf, &slice.ptr, slice.len, sizeof(buf), &state
			);
			if (mbsLen == (size_t)-1) err(EX_DATAERR, "%s", file->path);
			slice.len -= slice.ptr - table->slices[i].ptr;

			fwrite(buf, 1, mbsLen, stream);
			if (ferror(stream)) err(EX_IOERR, "%s", file->path);
		}
	}
	file->clean = file->log.state;

	fclose(stream);
	if (ferror(stream)) err(EX_IOERR, "%s", file->path);
}
