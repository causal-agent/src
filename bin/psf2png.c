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
#include <string.h>
#include <unistd.h>

#include "png.h"

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
			break; default:  return 1;
		}
	}
	if (!cols && str) cols = strlen(str);
	if (!cols) return 1;

	const char *path = NULL;
	if (optind < argc) path = argv[optind];
	
	FILE *file = path ? fopen(path, "r") : stdin;
	if (!file) err(1, "%s", path);
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
	if (ferror(file)) err(1, "%s", path);
	if (len < 1) errx(1, "%s: truncated header", path);

	uint32_t widthBytes = (header.glyph.width + 7) / 8;
	uint8_t glyphs[header.glyph.len][header.glyph.height][widthBytes];
	len = fread(glyphs, header.glyph.size, header.glyph.len, file);
	if (ferror(file)) err(1, "%s", path);
	if (len < header.glyph.len) {
		errx(1, "%s: truncated glyphs", path);
	}
	fclose(file);

	uint32_t count = (str ? strlen(str) : header.glyph.len);
	uint32_t width = header.glyph.width * cols;
	uint32_t rows = (count + cols - 1) / cols;
	uint32_t height = header.glyph.height * rows;

	pngHead(stdout, width, height, 8, PNGIndexed);
	uint8_t pal[] = {
		bg >> 16, bg >> 8, bg,
		fg >> 16, fg >> 8, fg,
	};
	pngPalette(stdout, pal, sizeof(pal));

	uint8_t data[height][1 + width];
	memset(data, PNGNone, sizeof(data));

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

	pngData(stdout, (uint8_t *)data, sizeof(data));
	pngTail(stdout);
}
