/* Backlight brightness control.
 *
 * Copyright (c) 2017, June McEnroe <programble@gmail.com>
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
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static const char *CLASS = "/sys/class/backlight";

int main(int argc, char *argv[]) {
    int error;

    if (argc < 2) errx(EX_USAGE, "usage: bri N | +... | -...");

    error = chdir(CLASS);
    if (error) err(EX_IOERR, "%s", CLASS);

    DIR *dir = opendir(".");
    if (!dir) err(EX_IOERR, "%s", CLASS);

    struct dirent *entry;
    while (NULL != (errno = 0, entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        error = chdir(entry->d_name);
        if (error) err(EX_IOERR, "%s", entry->d_name);
        break;
    }
    if (!entry) {
        if (errno) err(EX_IOERR, "%s", CLASS);
        errx(EX_CONFIG, "empty %s", CLASS);
    }

    char *value = argv[1];

    if (value[0] == '+' || value[0] == '-') {
        FILE *actual = fopen("actual_brightness", "r");
        if (!actual) err(EX_IOERR, "actual_brightness");

        unsigned int brightness;
        int match = fscanf(actual, "%u", &brightness);
        if (match == EOF) err(EX_IOERR, "actual_brightness");
        if (match < 1) err(EX_DATAERR, "actual_brightness");

        size_t count = strnlen(value, 15);
        if (value[0] == '+') {
            brightness += 16 * count;
        } else {
            brightness -= 16 * count;
        }

        char buf[15];
        snprintf(buf, sizeof(buf), "%u", brightness);

        value = buf;
    }

    FILE *brightness = fopen("brightness", "w");
    if (!brightness) err(EX_IOERR, "brightness");

    int count = fprintf(brightness, "%s", value);
    if (count < 0) err(EX_IOERR, "brightness");

    return EX_OK;
}
