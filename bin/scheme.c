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
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sysexits.h>
#include <unistd.h>
#include <zlib.h>

static const struct Hsv { double h, s, v; }
	R = {   0.0, 1.0, 1.0 },
	Y = {  60.0, 1.0, 1.0 },
	G = { 120.0, 1.0, 1.0 },
	C = { 180.0, 1.0, 1.0 },
	B = { 240.0, 1.0, 1.0 },
	M = { 300.0, 1.0, 1.0 };

static struct Hsv x(struct Hsv o, double hd, double sf, double vf) {
	return (struct Hsv) {
		fmod(o.h + hd, 360.0),
		fmin(o.s * sf, 1.0),
		fmin(o.v * vf, 1.0),
	};
}

static struct Rgb {
	uint8_t r, g, b;
} toRgb(struct Hsv hsv) {
	double c = hsv.v * hsv.s;
	double h = hsv.h / 60.0;
	double x = c * (1.0 - fabs(fmod(h, 2.0) - 1.0));
	double m = hsv.v - c;
	double r = m, g = m, b = m;
	if      (h <= 1.0) { r += c; g += x; }
	else if (h <= 2.0) { r += x; g += c; }
	else if (h <= 3.0) { g += c; b += x; }
	else if (h <= 4.0) { g += x; b += c; }
	else if (h <= 5.0) { r += x; b += c; }
	else if (h <= 6.0) { r += c; b += x; }
	return (struct Rgb) { r * 255.0, g * 255.0, b * 255.0 };
}

enum { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
static struct {
	struct Hsv dark[8];
	struct Hsv light[8];
	struct Hsv background, text, bold, selection, cursor;
} scheme;

static void generate(void) {
	scheme.light[BLACK]   = x(R, +45.0, 0.3, 0.3);
	scheme.light[RED]     = x(R, +10.0, 0.9, 0.8);
	scheme.light[GREEN]   = x(G, -55.0, 0.8, 0.6);
	scheme.light[YELLOW]  = x(Y, -20.0, 0.8, 0.8);
	scheme.light[BLUE]    = x(B, -55.0, 0.4, 0.5);
	scheme.light[MAGENTA] = x(M, +45.0, 0.4, 0.6);
	scheme.light[CYAN]    = x(C, -60.0, 0.3, 0.6);
	scheme.light[WHITE]   = x(R, +45.0, 0.3, 0.8);

	scheme.dark[BLACK] = x(scheme.light[BLACK], 0.0, 1.0, 0.3);
	scheme.dark[WHITE] = x(scheme.light[WHITE], 0.0, 1.0, 0.6);
	for (int i = RED; i < WHITE; ++i) {
		scheme.dark[i] = x(scheme.light[i], 0.0, 1.0, 0.8);
	}

	scheme.background = x(scheme.dark[BLACK],    0.0, 1.0, 0.9);
	scheme.text       = x(scheme.light[WHITE],   0.0, 1.0, 0.9);
	scheme.bold       = x(scheme.light[WHITE],   0.0, 1.0, 1.0);
	scheme.selection  = x(scheme.light[RED],   +10.0, 1.0, 0.8);
	scheme.cursor     = x(scheme.dark[WHITE],    0.0, 1.0, 0.8);
}

static void printHsv(struct Hsv hsv) {
	printf("%g,%g,%g\n", hsv.h, hsv.s, hsv.v);
}
static void hsv(bool ansi) {
	for (int i = BLACK; i <= WHITE; ++i) {
		printHsv(scheme.dark[i]);
	}
	for (int i = BLACK; i <= WHITE; ++i) {
		printHsv(scheme.light[i]);
	}
	if (ansi) return;
	printHsv(scheme.background);
	printHsv(scheme.text);
	printHsv(scheme.bold);
	printHsv(scheme.selection);
	printHsv(scheme.cursor);
}

static void printHex(struct Hsv hsv) {
	struct Rgb rgb = toRgb(hsv);
	printf("%02X%02X%02X\n", rgb.r, rgb.g, rgb.b);
}
static void hex(bool ansi) {
	for (int i = BLACK; i <= WHITE; ++i) {
		printHex(scheme.dark[i]);
	}
	for (int i = BLACK; i <= WHITE; ++i) {
		printHex(scheme.light[i]);
	}
	if (ansi) return;
	printHex(scheme.background);
	printHex(scheme.text);
	printHex(scheme.bold);
	printHex(scheme.selection);
	printHex(scheme.cursor);
}

static void console(void) {
	for (int i = BLACK; i <= WHITE; ++i) {
		struct Rgb rgb = toRgb(scheme.dark[i]);
		printf("\x1B]P%X%02X%02X%02X", i, rgb.r, rgb.g, rgb.b);
	}
	for (int i = BLACK; i <= WHITE; ++i) {
		struct Rgb rgb = toRgb(scheme.dark[i]);
		printf("\x1B]P%X%02X%02X%02X", 8 + i, rgb.r, rgb.g, rgb.b);
	}
}

static void printMintty(const char *key, struct Hsv hsv) {
	struct Rgb rgb = toRgb(hsv);
	printf("%s=%d,%d,%d\n", key, rgb.r, rgb.g, rgb.b);
}
static void mintty(void) {
	printMintty("Black", scheme.dark[BLACK]);
	printMintty("Red", scheme.dark[RED]);
	printMintty("Green", scheme.dark[GREEN]);
	printMintty("Yellow", scheme.dark[YELLOW]);
	printMintty("Blue", scheme.dark[BLUE]);
	printMintty("Magenta", scheme.dark[MAGENTA]);
	printMintty("Cyan", scheme.dark[CYAN]);
	printMintty("White", scheme.dark[WHITE]);

	printMintty("BoldBlack", scheme.light[BLACK]);
	printMintty("BoldRed", scheme.light[RED]);
	printMintty("BoldGreen", scheme.light[GREEN]);
	printMintty("BoldYellow", scheme.light[YELLOW]);
	printMintty("BoldBlue", scheme.light[BLUE]);
	printMintty("BoldMagenta", scheme.light[MAGENTA]);
	printMintty("BoldCyan", scheme.light[CYAN]);
	printMintty("BoldWhite", scheme.light[WHITE]);

	printMintty("BackgroundColour", scheme.background);
	printMintty("ForegroundColour", scheme.text);
	printMintty("CursorColour", scheme.cursor);
}

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

static void png(const struct Hsv *hsv, size_t len) {
	if (len > 256) len = 256;
	uint32_t swatchWidth = 64;
	uint32_t swatchHeight = 64;
	uint32_t columns = 8;
	uint32_t rows = (len + columns - 1) / columns;
	uint32_t width = swatchWidth * columns;
	uint32_t height = swatchHeight * rows;

	pngWrite("\x89PNG\r\n\x1A\n", 8);

	pngChunk("IHDR", 13);
	pngInt(width);
	pngInt(height);
	pngWrite("\x08\x03\0\0\0", 5);
	pngInt(crc);

	pngChunk("PLTE", 3 * len);
	for (size_t i = 0; i < len; ++i) {
		struct Rgb rgb = toRgb(hsv[i]);
		pngWrite(&rgb, 3);
	}
	pngInt(crc);

	uint8_t data[height][1 + width];
	memset(data, 0, sizeof(data));
	for (uint32_t y = 0; y < height; ++y) {
		enum { NONE, SUB, UP, AVERAGE, PAETH };
		data[y][0] = (y % swatchHeight) ? UP : SUB;
	}
	for (size_t i = 0; i < len; ++i) {
		uint32_t y = swatchHeight * (i / columns);
		uint32_t x = swatchWidth * (i % columns);
		data[y][1 + x] = x ? 1 : i;
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

int main(int argc, char *argv[]) {
	generate();
	bool ansi = true;
	char out = 'x';
	int opt;
	while (0 < (opt = getopt(argc, argv, "aghlmtx"))) {
		switch (opt) {
			break; case 'a': ansi = true;
			break; case 't': ansi = false;
			break; case '?': return EX_USAGE;
			break; default: out = opt;
		}
	}
	switch (out) {
		break; case 'g': png((struct Hsv *)&scheme, (ansi ? 16 : 21));
		break; case 'h': hsv(ansi);
		break; case 'l': console();
		break; case 'm': mintty();
		break; case 'x': hex(ansi);
	}
	return EX_OK;
}
