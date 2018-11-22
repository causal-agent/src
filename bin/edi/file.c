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
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <wchar.h>

#include "edi.h"

enum {
	BufCap = 8192,
	LogCap = 8,
};

struct File fileAlloc(char *path) {
	struct File file = {
		.path = path,
		.buf = bufferAlloc(BufCap),
		.log = logAlloc(LogCap),
		.clean = 0,
	};
	if (!path) logPush(&file.log, TableEmpty);
	return file;
}

void fileFree(struct File *file) {
	logFree(&file->log);
	bufferFree(&file->buf);
	free(file->path);
}

static const mbstate_t MBStateInit;

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

	struct Table table = tableAlloc(1);

	char buf[BufCap];
	mbstate_t mbState = MBStateInit;
	while (!feof(stream)) {
		size_t mbsLen = fread(buf, 1, sizeof(buf), stream);
		if (ferror(stream)) err(EX_IOERR, "%s", file->path);

		const char *mbs = buf;
		mbstate_t mbLenState = mbState;
		size_t wcsLen = mbsnrtowcs(NULL, &mbs, mbsLen, 0, &mbLenState);
		if (wcsLen == (size_t)-1) err(EX_DATAERR, "%s", file->path);

		wchar_t *wcs = bufferDest(&file->buf, wcsLen);
		assert(wcsLen == mbsnrtowcs(wcs, &mbs, mbsLen, wcsLen, &mbState));

		tablePush(&table, file->buf.slice);
	}
	logPush(&file->log, table);
	file->clean = file->log.state;

	fclose(stream);
}
