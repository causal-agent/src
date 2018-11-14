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

#include <arpa/inet.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <zlib.h>

enum {
	PNGGrayscale,
	PNGTruecolor = 2,
	PNGIndexed,
	PNGAlpha,
};

enum {
	PNGNone,
	PNGSub,
	PNGUp,
	PNGAverage,
	PNGPaeth,
};

static uint32_t pngCRC;
static inline void pngWrite(FILE *file, const void *ptr, size_t len) {
	if (!fwrite(ptr, len, 1, file)) err(EX_IOERR, "pngWrite");
	pngCRC = crc32(pngCRC, ptr, len);
}
static inline void pngInt32(FILE *file, uint32_t n) {
	n = htonl(n);
	pngWrite(file, &n, 4);
}
static inline void pngChunk(FILE *file, char type[static 4], uint32_t len) {
	pngInt32(file, len);
	pngCRC = crc32(0, Z_NULL, 0);
	pngWrite(file, type, 4);
}

static inline void pngHead(
	FILE *file, uint32_t width, uint32_t height, uint8_t depth, uint8_t color
) {
	pngWrite(file, "\x89PNG\r\n\x1A\n", 8);
	pngChunk(file, "IHDR", 13);
	pngInt32(file, width);
	pngInt32(file, height);
	pngWrite(file, &depth, 1);
	pngWrite(file, &color, 1);
	pngWrite(file, "\0\0\0", 3);
	pngInt32(file, pngCRC);
}

static inline void pngPalette(FILE *file, const uint8_t *pal, uint32_t len) {
	pngChunk(file, "PLTE", len);
	pngWrite(file, pal, len);
	pngInt32(file, pngCRC);
}

static inline void pngData(FILE *file, const uint8_t *data, uint32_t len) {
	uLong zlen = compressBound(len);
	uint8_t *zdata = malloc(zlen);
	if (!zlen) err(EX_OSERR, "malloc");

	int error = compress(zdata, &zlen, data, len);
	if (error != Z_OK) errx(EX_SOFTWARE, "compress: %d", error);

	pngChunk(file, "IDAT", zlen);
	pngWrite(file, zdata, zlen);
	pngInt32(file, pngCRC);

	free(zdata);
}

static inline void pngTail(FILE *file) {
	pngChunk(file, "IEND", 0);
	pngInt32(file, pngCRC);
}
