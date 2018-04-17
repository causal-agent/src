/* Copyright (c) 2017, June McEnroe <programble@gmail.com>
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

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

static bool zero(const uint8_t *ptr, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (ptr[i]) return false;
    }
    return true;
}

static struct {
    size_t cols;
    size_t group;
    bool ascii;
    bool offset;
    bool skip;
} options = { 16, 8, true, true, false };

static void dump(FILE *file) {
    bool skip = false;

    uint8_t buf[options.cols];
    size_t offset = 0;
    for (
        size_t size;
        (size = fread(buf, 1, sizeof(buf), file));
        offset += size
    ) {
        if (options.skip) {
            if (zero(buf, size)) {
                if (!skip) printf("*\n");
                skip = true;
                continue;
            } else {
                skip = false;
            }
        }

        if (options.offset) {
            printf("%08zX:  ", offset);
        }

        for (size_t i = 0; i < sizeof(buf); ++i) {
            if (options.group) {
                if (i && !(i % options.group)) {
                    printf(" ");
                }
            }
            if (i < size) {
                printf("%02hhX ", buf[i]);
            } else {
                printf("   ");
            }
        }

        if (options.ascii) {
            printf(" ");
            for (size_t i = 0; i < size; ++i) {
                if (options.group) {
                    if (i && !(i % options.group)) {
                        printf(" ");
                    }
                }
                printf("%c", isprint(buf[i]) ? buf[i] : '.');
            }
        }

        printf("\n");
    }
}

static void undump(FILE *file) {
    uint8_t byte;
    int match;
    while (0 < (match = fscanf(file, " %hhx", &byte))) {
        printf("%c", byte);
    }
    if (!match) errx(EX_DATAERR, "invalid input");
}

int main(int argc, char *argv[]) {
    bool reverse = false;
    const char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "ac:g:rsz"))) {
        switch (opt) {
            case 'a': options.ascii ^= true; break;
            case 'c': options.cols = strtoul(optarg, NULL, 0); break;
            case 'g': options.group = strtoul(optarg, NULL, 0); break;
            case 'r': reverse = true; break;
            case 's': options.offset ^= true; break;
            case 'z': options.skip ^= true; break;
            default: return EX_USAGE;
        }
    }
    if (argc > optind) path = argv[optind];
    if (!options.cols) return EX_USAGE;

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) err(EX_NOINPUT, "%s", path);

    if (reverse) {
        undump(file);
    } else {
        dump(file);
    }
    if (ferror(file)) err(EX_IOERR, "%s", path);

    return EX_OK;
}
