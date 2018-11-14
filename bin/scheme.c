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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "png.h"

typedef unsigned uint;
typedef unsigned char byte;

static const struct HSV { double h, s, v; }
	R = {   0.0, 1.0, 1.0 },
	Y = {  60.0, 1.0, 1.0 },
	G = { 120.0, 1.0, 1.0 },
	C = { 180.0, 1.0, 1.0 },
	B = { 240.0, 1.0, 1.0 },
	M = { 300.0, 1.0, 1.0 };

static struct RGB { byte r, g, b; } toRGB(struct HSV hsv) {
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
	return (struct RGB) { r * 255.0, g * 255.0, b * 255.0 };
}

static struct HSV x(struct HSV o, double hd, double sf, double vf) {
	return (struct HSV) {
		fmod(o.h + hd, 360.0),
		fmin(o.s * sf, 1.0),
		fmin(o.v * vf, 1.0),
	};
}

enum {
	Black,
	Red,
	Green,
	Yellow,
	Blue,
	Magenta,
	Cyan,
	White,
	Dark = 0,
	Light = 8,
	Background = 16,
	Foreground,
	Bold,
	Selection,
	Cursor,
	SchemeLen,
};
static struct HSV scheme[SchemeLen];

static void generate(void) {
	scheme[Light + Black]   = x(R, +45.0, 0.3, 0.3);
	scheme[Light + Red]     = x(R, +10.0, 0.9, 0.8);
	scheme[Light + Green]   = x(G, -55.0, 0.8, 0.6);
	scheme[Light + Yellow]  = x(Y, -20.0, 0.8, 0.8);
	scheme[Light + Blue]    = x(B, -55.0, 0.4, 0.5);
	scheme[Light + Magenta] = x(M, +45.0, 0.4, 0.6);
	scheme[Light + Cyan]    = x(C, -60.0, 0.3, 0.6);
	scheme[Light + White]   = x(R, +45.0, 0.3, 0.8);

	scheme[Dark + Black] = x(scheme[Light + Black], 0.0, 1.0, 0.3);
	scheme[Dark + White] = x(scheme[Light + White], 0.0, 1.0, 0.6);
	for (uint i = Red; i < White; ++i) {
		scheme[Dark + i] = x(scheme[Light + i], 0.0, 1.0, 0.8);
	}

	scheme[Background] = x(scheme[Dark + Black],    0.0, 1.0, 0.9);
	scheme[Foreground] = x(scheme[Light + White],   0.0, 1.0, 0.9);
	scheme[Bold]       = x(scheme[Light + White],   0.0, 1.0, 1.0);
	scheme[Selection]  = x(scheme[Light + Red],   +10.0, 1.0, 0.8);
	scheme[Cursor]     = x(scheme[Dark + White],    0.0, 1.0, 0.8);
}

static void swap(uint a, uint b) {
	struct HSV t = scheme[a];
	scheme[a] = scheme[b];
	scheme[b] = t;
}

static void invert(void) {
	swap(Dark + Black, Light + White);
	swap(Light + Black, Dark + White);
}

static void printHSV(uint n) {
	printf("%g,%g,%g\n", scheme[n].h, scheme[n].s, scheme[n].v);
}

static void printRGB(uint n) {
	struct RGB rgb = toRGB(scheme[n]);
	printf("%02hhX%02hhX%02hhX\n", rgb.r, rgb.g, rgb.b);
}

static const char *CNames[SchemeLen] = {
	[Dark + Black]    = "DarkBlack",
	[Dark + Red]      = "DarkRed",
	[Dark + Green]    = "DarkGreen",
	[Dark + Yellow]   = "DarkYellow",
	[Dark + Blue]     = "DarkBlue",
	[Dark + Magenta]  = "DarkMagenta",
	[Dark + Cyan]     = "DarkCyan",
	[Dark + White]    = "DarkWhite",
	[Light + Black]   = "LightBlack",
	[Light + Red]     = "LightRed",
	[Light + Green]   = "LightGreen",
	[Light + Yellow]  = "LightYellow",
	[Light + Blue]    = "LightBlue",
	[Light + Magenta] = "LightMagenta",
	[Light + Cyan]    = "LightCyan",
	[Light + White]   = "LightWhite",
	[Background]      = "Background",
	[Foreground]      = "Foreground",
	[Bold]            = "Bold",
	[Selection]       = "Selection",
	[Cursor]          = "Cursor",
};
static void printCHead(void) {
	printf("enum {\n");
}
static void printC(uint n) {
	struct RGB rgb = toRGB(scheme[n]);
	printf("\t%s = 0x%02hhX%02hhX%02hhX,\n", CNames[n], rgb.r, rgb.g, rgb.b);
}
static void printCTail(void) {
	printf("};\n");
}

static void printLinux(uint n) {
	struct RGB rgb = toRGB(scheme[n]);
	printf("\x1B]P%X%02hhX%02hhX%02hhX", n, rgb.r, rgb.g, rgb.b);
}

static const char *MinttyNames[SchemeLen] = {
	[Dark + Black]    = "Black",
	[Dark + Red]      = "Red",
	[Dark + Green]    = "Green",
	[Dark + Yellow]   = "Yellow",
	[Dark + Blue]     = "Blue",
	[Dark + Magenta]  = "Magenta",
	[Dark + Cyan]     = "Cyan",
	[Dark + White]    = "White",
	[Light + Black]   = "BoldBlack",
	[Light + Red]     = "BoldRed",
	[Light + Green]   = "BoldGreen",
	[Light + Yellow]  = "BoldYellow",
	[Light + Blue]    = "BoldBlue",
	[Light + Magenta] = "BoldMagenta",
	[Light + Cyan]    = "BoldCyan",
	[Light + White]   = "BoldWhite",
	[Background]      = "BackgroundColour",
	[Foreground]      = "ForegroundColour",
	[Cursor]          = "CursorColour",
};
static void printMintty(uint n) {
	if (!MinttyNames[n]) return;
	struct RGB rgb = toRGB(scheme[n]);
	printf("%s=%hhd,%hhd,%hhd\n", MinttyNames[n], rgb.r, rgb.g, rgb.b);
}

static void png(uint at, uint to) {
	if (to - at > 256) to = at + 256;

	uint len = to - at;
	uint swatchWidth = 64;
	uint swatchHeight = 64;
	uint cols = 8;
	uint rows = (len + cols - 1) / cols;
	uint width = swatchWidth * cols;
	uint height = swatchHeight * rows;

	pngHead(stdout, width, height, 8, PNGIndexed);

	struct RGB rgb[len];
	for (uint i = 0; i < len; ++i) {
		rgb[i] = toRGB(scheme[at + i]);
	}
	pngPalette(stdout, (byte *)rgb, sizeof(rgb));

	uint8_t data[height][1 + width];
	memset(data, 0, sizeof(data));
	for (uint32_t y = 0; y < height; ++y) {
		data[y][0] = (y % swatchHeight) ? PNGUp : PNGSub;
	}
	for (uint i = at; i < to; ++i) {
		uint p = i - at;
		uint32_t y = swatchHeight * (p / cols);
		uint32_t x = swatchWidth * (p % cols);
		data[y][1 + x] = x ? 1 : p;
	}

	pngData(stdout, (byte *)data, sizeof(data));
	pngTail(stdout);
}

static void print(void (*fn)(uint), uint at, uint to) {
	for (uint i = at; i < to; ++i) {
		fn(i);
	}
}

int main(int argc, char *argv[]) {
	generate();
	uint at = 0;
	uint to = Background;
	char out = 'x';

	int opt;
	while (0 < (opt = getopt(argc, argv, "acghilmp:tx"))) {
		switch (opt) {
			break; case 'a': to = Background;
			break; case 'i': invert();
			break; case 'p': at = strtoul(optarg, NULL, 0); to = at + 1;
			break; case 't': to = SchemeLen;
			break; case '?': return EX_USAGE;
			break; default: out = opt;
		}
	}

	switch (out) {
		break; case 'c': printCHead(); print(printC, at, to); printCTail();
		break; case 'g': png(at, to);
		break; case 'h': print(printHSV, at, to);
		break; case 'l': print(printLinux, at, to);
		break; case 'm': print(printMintty, at, to);
		break; case 'x': print(printRGB, at, to);
	}

	return EX_OK;
}
