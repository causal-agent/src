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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "edi.h"

static void errorExit(enum Error error, const char *prefix) {
	if (error > Errno) errc(EX_IOERR, error - Errno, "%s", prefix);
	else errx(EX_DATAERR, "%s: %d", prefix, error);
}

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

	if (argc < 2) return EX_USAGE;

	enum Error error;

	struct File file = fileAlloc(strdup(argv[1]));
	error = fileRead(&file);
	if (error) errorExit(error, file.path);

	FILE *store = fopen("store.edi", "w");
	if (!store) err(EX_CANTCREAT, "store.edi");

	error = storeWrite(store, &file.edit);
	if (error) errorExit(error, "store.edi");

	fclose(store);
	if (ferror(store)) err(EX_IOERR, "store.edi");

	store = fopen("store.edi", "r");
	if (!store) err(EX_CANTCREAT, "store.edi");

	error = storeRead(store, &file.edit);
	if (error) errorExit(error, "store.edi");

	const struct Table *table = logTable(&file.edit.log);
	for (struct Iter it = iter(table, 0); it.ch != WEOF; it = iterNext(it)) {
		printf("%lc", it.ch);
	}

	error = fileWrite(&file);
	if (error) errorExit(error, file.path);

	fileFree(&file);
}
