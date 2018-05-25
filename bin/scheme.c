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

struct Ansi {
	enum { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
	struct Hsv dark[8];
	struct Hsv light[8];
};

struct Terminal {
	struct Ansi ansi;
	struct Hsv background, text, bold, selection, cursor;
};

#define HSV_LEN(x) (sizeof(x) / sizeof(struct Hsv))
struct Scheme {
	size_t len;
	union {
		struct Hsv hsv;
		struct Ansi ansi;
		struct Terminal terminal;
	};
};

static struct Scheme ansi(void) {
	struct Ansi a = {
		.light = {
			[BLACK]   = x(R, +45.0, 0.3, 0.3),
			[RED]     = x(R, +10.0, 0.9, 0.8),
			[GREEN]   = x(G, -55.0, 0.8, 0.6),
			[YELLOW]  = x(Y, -20.0, 0.8, 0.8),
			[BLUE]    = x(B, -55.0, 0.4, 0.5),
			[MAGENTA] = x(M, +45.0, 0.4, 0.6),
			[CYAN]    = x(C, -60.0, 0.3, 0.6),
			[WHITE]   = x(R, +45.0, 0.3, 0.8),
		},
	};
	a.dark[BLACK] = x(a.light[BLACK], 0.0, 1.0, 0.3);
	a.dark[WHITE] = x(a.light[WHITE], 0.0, 1.0, 0.6);
	for (int i = RED; i < WHITE; ++i) {
		a.dark[i] = x(a.light[i], 0.0, 1.0, 0.8);
	}
	return (struct Scheme) { .len = HSV_LEN(a), .ansi = a };
}

static struct Scheme terminal(void) {
	struct Ansi a = ansi().ansi;
	struct Terminal t = {
		.ansi       = a,
		.background = x(a.dark[BLACK],    0.0, 1.0, 0.9),
		.text       = x(a.light[WHITE],   0.0, 1.0, 0.9),
		.bold       = x(a.light[WHITE],   0.0, 1.0, 1.0),
		.selection  = x(a.light[RED],   +10.0, 1.0, 0.8),
		.cursor     = x(a.dark[WHITE],    0.0, 1.0, 0.8),
	};
	return (struct Scheme) { .len = HSV_LEN(t), .terminal = t };
}

static void hsv(const struct Hsv *hsv, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		printf("%g,%g,%g\n", hsv[i].h, hsv[i].s, hsv[i].v);
	}
}

struct Rgb { uint8_t r, g, b; };
static struct Rgb toRgb(struct Hsv hsv) {
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

static void hex(const struct Hsv *hsv, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		struct Rgb rgb = toRgb(hsv[i]);
		printf("%02X%02X%02X\n", rgb.r, rgb.g, rgb.b);
	}
}

static void linux(const struct Hsv *hsv, size_t len) {
	if (len > 16) len = 16;
	for (size_t i = 0; i < len; ++i) {
		struct Rgb rgb = toRgb(hsv[i]);
		printf("\x1B]P%zX%02X%02X%02X", i, rgb.r, rgb.g, rgb.b);
	}
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
	struct Scheme (*gen)(void) = ansi;
	void (*out)(const struct Hsv *, size_t len) = hex;
	int opt;
	while (0 < (opt = getopt(argc, argv, "aghltx"))) {
		switch (opt) {
			break; case 'a': gen = ansi;
			break; case 'g': out = png;
			break; case 'h': out = hsv;
			break; case 'l': out = linux;
			break; case 't': gen = terminal;
			break; case 'x': out = hex;
			break; default: return EX_USAGE;
		}
	}
	struct Scheme scheme = gen();
	out(&scheme.hsv, scheme.len);
	return EX_OK;
}
