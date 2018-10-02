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
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <zlib.h>

static uint32_t crc;
static void pngWrite(const void *ptr, size_t size) {
	fwrite(ptr, size, 1, stdout);
	if (ferror(stdout)) err(EX_IOERR, "(stdout)");
	crc = crc32(crc, ptr, size);
}
static void pngInt(uint32_t host) {
	uint32_t net = htonl(host);
	pngWrite(&net, 4);
}
static void pngChunk(const char *type, uint32_t size) {
	pngInt(size);
	crc = crc32(0, Z_NULL, 0);
	pngWrite(type, 4);
}

int main(int argc, char *argv[]) {
	uint32_t cols = 32;
	const char *str = NULL;
	uint32_t fg = 0xFFFFFF;
	uint32_t bg = 0x000000;

	int opt;
	while (0 < (opt = getopt(argc, argv, "b:c:f:s:"))) {
		switch (opt) {
			break; case 'b': bg = strtoul(optarg, NULL, 16);
			break; case 'c': cols = strtoul(optarg, NULL, 0);
			break; case 'f': fg = strtoul(optarg, NULL, 16);
			break; case 's': str = optarg;
			break; default:  return EX_USAGE;
		}
	}
	if (!cols && str) cols = strlen(str);
	if (!cols) return EX_USAGE;

	const char *path = NULL;
	if (optind < argc) path = argv[optind];
	
	FILE *file = path ? fopen(path, "r") : stdin;
	if (!file) err(EX_NOINPUT, "%s", path);
	if (!path) path = "(stdin)";

	struct {
		uint32_t magic;
		uint32_t version;
		uint32_t size;
		uint32_t flags;
		struct {
			uint32_t len;
			uint32_t size;
			uint32_t height;
			uint32_t width;
		} glyph;
	} header;
	size_t len = fread(&header, sizeof(header), 1, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);
	if (len < 1) errx(EX_DATAERR, "%s: truncated header", path);

	uint32_t widthBytes = (header.glyph.width + 7) / 8;
	uint8_t glyphs[header.glyph.len][header.glyph.height][widthBytes];
	len = fread(glyphs, header.glyph.size, header.glyph.len, file);
	if (ferror(file)) err(EX_IOERR, "%s", path);
	if (len < header.glyph.len) {
		errx(EX_DATAERR, "%s: truncated glyphs", path);
	}
	fclose(file);

	pngWrite("\x89PNG\r\n\x1A\n", 8);

	uint32_t count = (str ? strlen(str) : header.glyph.len);
	uint32_t width = header.glyph.width * cols;
	uint32_t rows = (count + cols - 1) / cols;
	uint32_t height = header.glyph.height * rows;

	pngChunk("IHDR", 13);
	pngInt(width);
	pngInt(height);
	pngWrite("\x08\x03\x00\x00\x00", 5);
	pngInt(crc);

	pngChunk("PLTE", 6);
	pngWrite((uint8_t[]) { bg >> 16, bg >> 8, bg }, 3);
	pngWrite((uint8_t[]) { fg >> 16, fg >> 8, fg }, 3);
	pngInt(crc);

	uint8_t data[height][1 + width];
	memset(data, 0, sizeof(data));

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t row = header.glyph.height * (i / cols);
		uint32_t col = 1 + header.glyph.width * (i % cols);
		uint32_t g = (str ? str[i] : i);
		for (uint32_t y = 0; y < header.glyph.height; ++y) {
			for (uint32_t x = 0; x < header.glyph.width; ++x) {
				uint8_t bit = glyphs[g][y][x / 8] >> (7 - x % 8) & 1;
				data[row + y][col + x] = bit;
			}
		}
	}

	uLong size = compressBound(sizeof(data));
	uint8_t deflate[size];
	int error = compress(deflate, &size, (Byte *)data, sizeof(data));
	if (error != Z_OK) errx(EX_SOFTWARE, "compress: %d", error);

	pngChunk("IDAT", size);
	pngWrite(deflate, size);
	pngInt(crc);

	pngChunk("IEND", 0);
	pngInt(crc);
}
