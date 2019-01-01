/* Copyright (C) 2017  June McEnroe <june@causal.agency>
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

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned uint;

static const char *Class = "/sys/class/backlight";

int main(int argc, char *argv[]) {
	int error;

	const char *input = (argc > 1) ? argv[1] : NULL;

	error = chdir(Class);
	if (error) err(EX_OSFILE, "%s", Class);

	DIR *dir = opendir(".");
	if (!dir) err(EX_OSFILE, "%s", Class);

	struct dirent *entry;
	while (NULL != (errno = 0, entry = readdir(dir))) {
		if (entry->d_name[0] == '.') continue;

		error = chdir(entry->d_name);
		if (error) err(EX_OSFILE, "%s/%s", Class, entry->d_name);
		break;
	}
	if (!entry) {
		if (errno) err(EX_IOERR, "%s", Class);
		errx(EX_CONFIG, "%s: empty", Class);
	}

	FILE *actual = fopen("actual_brightness", "r");
	if (!actual) err(EX_OSFILE, "actual_brightness");

	uint value;
	int match = fscanf(actual, "%u", &value);
	if (match == EOF) err(EX_IOERR, "actual_brightness");
	if (match < 1) errx(EX_DATAERR, "actual_brightness");

	if (!input) {
		printf("%u\n", value);
		return EX_OK;
	}

	if (input[0] == '+' || input[0] == '-') {
		size_t count = strnlen(input, 16);
		if (input[0] == '+') {
			value += 16 * count;
		} else {
			value -= 16 * count;
		}
	} else {
		value = strtoul(input, NULL, 0);
	}

	FILE *brightness = fopen("brightness", "w");
	if (!brightness) err(EX_OSFILE, "brightness");

	int size = fprintf(brightness, "%u", value);
	if (size < 0) err(EX_IOERR, "brightness");

	return EX_OK;
}
