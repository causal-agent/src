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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

static inline uint32_t pngCRCTable(uint8_t n) {
	static uint32_t table[256];
	if (table[1]) return table[n];
	for (int i = 0; i < 256; ++i) {
		table[i] = i;
		for (int j = 0; j < 8; ++j) {
			table[i] = (table[i] >> 1) ^ (table[i] & 1 ? 0xEDB88320 : 0);
		}
	}
	return table[n];
}

static uint32_t pngCRC;

static inline void pngWrite(FILE *file, const uint8_t *ptr, uint32_t len) {
	if (!fwrite(ptr, len, 1, file)) err(EX_IOERR, "pngWrite");
	for (uint32_t i = 0; i < len; ++i) {
		pngCRC = pngCRCTable(pngCRC ^ ptr[i]) ^ (pngCRC >> 8);
	}
}
static inline void pngInt32(FILE *file, uint32_t n) {
	pngWrite(file, (uint8_t []) { n >> 24, n >> 16, n >> 8, n }, 4);
}
static inline void pngChunk(FILE *file, char type[static 4], uint32_t len) {
	pngInt32(file, len);
	pngCRC = ~0;
	pngWrite(file, (uint8_t *)type, 4);
}

enum {
	PNGGrayscale,
	PNGTruecolor = 2,
	PNGIndexed,
	PNGAlpha,
};

static inline void pngHead(
	FILE *file, uint32_t width, uint32_t height, uint8_t depth, uint8_t color
) {
	pngWrite(file, (uint8_t *)"\x89PNG\r\n\x1A\n", 8);
	pngChunk(file, "IHDR", 13);
	pngInt32(file, width);
	pngInt32(file, height);
	pngWrite(file, &depth, 1);
	pngWrite(file, &color, 1);
	pngWrite(file, (uint8_t []) { 0, 0, 0 }, 3);
	pngInt32(file, ~pngCRC);
}

static inline void pngPalette(FILE *file, const uint8_t *pal, uint32_t len) {
	pngChunk(file, "PLTE", len);
	pngWrite(file, pal, len);
	pngInt32(file, ~pngCRC);
}

enum {
	PNGNone,
	PNGSub,
	PNGUp,
	PNGAverage,
	PNGPaeth,
};

static inline void pngData(FILE *file, const uint8_t *data, uint32_t len) {
	uint32_t adler1 = 1, adler2 = 0;
	for (uint32_t i = 0; i < len; ++i) {
		adler1 = (adler1 + data[i]) % 65521;
		adler2 = (adler1 + adler2) % 65521;
	}
	uint32_t zlen = 2 + 5 * ((len + 0xFFFE) / 0xFFFF) + len + 4;
	pngChunk(file, "IDAT", zlen);
	pngWrite(file, (uint8_t []) { 0x08, 0x1D }, 2);
	for (; len > 0xFFFF; data += 0xFFFF, len -= 0xFFFF) {
		pngWrite(file, (uint8_t []) { 0x00, 0xFF, 0xFF, 0x00, 0x00 }, 5);
		pngWrite(file, data, 0xFFFF);
	}
	pngWrite(file, (uint8_t []) { 0x01, len, len >> 8, ~len, ~len >> 8 }, 5);
	pngWrite(file, data, len);
	pngInt32(file, adler2 << 16 | adler1);
	pngInt32(file, ~pngCRC);
}

static inline void pngTail(FILE *file) {
	pngChunk(file, "IEND", 0);
	pngInt32(file, ~pngCRC);
}
