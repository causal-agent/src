/* Copyright (c) 2018, June McEnroe <programble@gmail.com>
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
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static const uint8_t SIGNATURE[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
static const uint8_t IEND[] = { 'I', 'E', 'N', 'D' };

static unsigned counter;
static char path[FILENAME_MAX];
static int fd;

static void next(const char *prefix) {
    if (counter++) {
        int error = close(fd);
        if (error) err(EX_IOERR, "%s", path);
    }
    snprintf(path, sizeof(path), "%s%04u.png", prefix, counter);
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) err(EX_CANTCREAT, "%s", path);
    printf("%s\n", path);
}

static void writeAll(const uint8_t *buf, size_t size) {
    while (size) {
        ssize_t writeSize = write(fd, buf, size);
        if (writeSize < 0) err(EX_IOERR, "%s", path);
        buf += writeSize;
        size -= writeSize;
    }
}

int main(int argc, char *argv[]) {
    const char *prefix = (argc > 1) ? argv[1] : "";

    for (;;) {
        uint8_t buf[4096];
        ssize_t size = read(STDIN_FILENO, buf, sizeof(buf));
        if (size < 0) err(EX_IOERR, "read");
        if (!size) return EX_OK;

        const uint8_t *signature = memmem(buf, size, SIGNATURE, sizeof(SIGNATURE));
        if (signature) {
            writeAll(buf, signature - buf);
            next(prefix);
            writeAll(signature, size - (signature - buf));
        } else {
            const uint8_t *iend = memmem(buf, size, IEND, sizeof(IEND));
            if (iend && iend - buf < size - 8) {
                warnx("trailing data, a PNG may be skipped");
            }
            writeAll(buf, size);
        }
    }
}
