/* Copyright (C) 2022  June McEnroe <june@causal.agency>
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

int main(int argc, char *argv[]) {
	if (argc < 2) return EX_USAGE;

	FILE *file = fopen(argv[1], "r");
	if (!file) err(EX_NOINPUT, "%s", argv[1]);

	size_t cap = 0x10000;
	uint8_t *buf = malloc(cap);
	if (!buf) err(EX_OSERR, "malloc");

	size_t len = 0;
	for (size_t n; 0 < (n = fread(&buf[len], 1, cap - len, file)); len += n) {
		buf = realloc(buf, (cap *= 2));
		if (!buf) err(EX_OSERR, "realloc");
	}

	unsigned nr = 0;
	for (
		uint8_t *ptr = buf;
		NULL != (ptr = memmem(ptr, &buf[len] - ptr, "icns", 4));
		ptr += 4
	) {
		if (&ptr[8] > &buf[len]) break;
		size_t len = ptr[4] << 24 | ptr[5] << 16 | ptr[6] << 8 | ptr[7];

		char path[64];
		snprintf(path, sizeof(path), "icon%04u.icns", ++nr);
		printf("%s\n", path);

		FILE *icon = fopen(path, "w");
		if (!icon) err(EX_CANTCREAT, "%s", path);

		size_t n = fwrite(ptr, 8 + len, 1, icon);
		if (!n) err(EX_IOERR, "%s", path);

		int error = fclose(icon);
		if (error) err(EX_IOERR, "%s", path);
	}
}
