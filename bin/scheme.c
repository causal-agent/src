/* Copyright (C) 2018, 2019  June McEnroe <june@causal.agency>
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
#include <unistd.h>

#include "png.h"

typedef unsigned uint;
typedef unsigned char byte;

struct HSV {
	double h, s, v;
};

struct RGB {
	byte r, g, b;
};

static struct RGB convert(struct HSV o) {
	double c = o.v * o.s;
	double h = o.h / 60.0;
	double x = c * (1.0 - fabs(fmod(h, 2.0) - 1.0));
	double m = o.v - c;
	double r = m, g = m, b = m;
	if      (h <= 1.0) { r += c; g += x; }
	else if (h <= 2.0) { r += x; g += c; }
	else if (h <= 3.0) { g += c; b += x; }
	else if (h <= 4.0) { g += x; b += c; }
	else if (h <= 5.0) { r += x; b += c; }
	else if (h <= 6.0) { r += c; b += x; }
	return (struct RGB) { r * 255.0, g * 255.0, b * 255.0 };
}

static const struct HSV
R = {   0.0, 1.0, 1.0 },
Y = {  60.0, 1.0, 1.0 },
G = { 120.0, 1.0, 1.0 },
C = { 180.0, 1.0, 1.0 },
B = { 240.0, 1.0, 1.0 },
M = { 300.0, 1.0, 1.0 };

static struct HSV x(struct HSV o, double hd, double sf, double vf) {
	return (struct HSV) {
		fmod(o.h + hd, 360.0),
		fmin(o.s * sf, 1.0),
		fmin(o.v * vf, 1.0),
	};
}

enum {
	Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
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
static struct HSV *dark = &scheme[Dark];
static struct HSV *light = &scheme[Light];

static void generate(void) {
	light[Black]   = x(R, +45.0, 0.3, 0.3);
	light[Red]     = x(R, +10.0, 0.9, 0.8);
	light[Green]   = x(G, -55.0, 0.8, 0.6);
	light[Yellow]  = x(Y, -20.0, 0.8, 0.8);
	light[Blue]    = x(B, -55.0, 0.4, 0.5);
	light[Magenta] = x(M, +45.0, 0.4, 0.6);
	light[Cyan]    = x(C, -60.0, 0.3, 0.6);
	light[White]   = x(R, +45.0, 0.3, 0.8);

	dark[Black] = x(light[Black], 0.0, 1.0, 0.3);
	dark[White] = x(light[White], 0.0, 1.0, 0.75);
	for (uint i = Red; i < White; ++i) {
		dark[i] = x(light[i], 0.0, 1.0, 0.8);
	}

	scheme[Background] = x(dark[Black],  0.0, 1.0, 0.9);
	scheme[Foreground] = x(light[White], 0.0, 1.0, 0.9);
	scheme[Bold]       = x(light[White], 0.0, 1.0, 1.0);
	scheme[Selection]  = x(light[Red], +10.0, 1.0, 0.8);
	scheme[Cursor]     = x(dark[White],  0.0, 1.0, 0.8);
}

static void swap(struct HSV *a, struct HSV *b) {
	struct HSV c = *a;
	*a = *b;
	*b = c;
}

static void invert(void) {
	swap(&dark[Black], &light[White]);
	swap(&dark[White], &light[Black]);
}

typedef void OutputFn(const struct HSV *hsv, uint len);

static void outputHSV(const struct HSV *hsv, uint len) {
	for (uint i = 0; i < len; ++i) {
		printf("%g,%g,%g\n", hsv[i].h, hsv[i].s, hsv[i].v);
	}
}

#define FORMAT_RGB "%02hhX%02hhX%02hhX"

static void outputRGB(const struct HSV *hsv, uint len) {
	for (uint i = 0; i < len; ++i) {
		struct RGB rgb = convert(hsv[i]);
		printf(FORMAT_RGB "\n", rgb.r, rgb.g, rgb.b);
	}
}

static void outputLinux(const struct HSV *hsv, uint len) {
	for (uint i = 0; i < len; ++i) {
		struct RGB rgb = convert(hsv[i]);
		printf("\x1B]P%X" FORMAT_RGB, i, rgb.r, rgb.g, rgb.b);
	}
}

static const char *Enum[SchemeLen] = {
	"DarkBlack", "DarkRed", "DarkGreen", "DarkYellow",
	"DarkBlue", "DarkMagenta", "DarkCyan", "DarkWhite",
	"LightBlack", "LightRed", "LightGreen", "LightYellow",
	"LightBlue", "LightMagenta", "LightCyan", "LightWhite",
	"Background", "Foreground", "Bold", "Selection", "Cursor",
};

static void outputEnum(const struct HSV *hsv, uint len) {
	printf("enum {\n");
	for (uint i = 0; i < len; ++i) {
		struct RGB rgb = convert(hsv[i]);
		printf("\t%s = 0x" FORMAT_RGB ",\n", Enum[i], rgb.r, rgb.g, rgb.b);
	}
	printf("};\n");
}

#define FORMAT_X "rgb:%02hhX/%02hhX/%02hhX"

static const char *Resources[SchemeLen] = {
	[Background] = "background",
	[Foreground] = "foreground",
	[Bold] = "colorBD",
	[Selection] = "highlightColor",
	[Cursor] = "cursorColor",
};

static void outputXTerm(const struct HSV *hsv, uint len) {
	for (uint i = 0; i < len; ++i) {
		struct RGB rgb = convert(hsv[i]);
		if (Resources[i]) {
			printf("XTerm*%s: " FORMAT_X "\n", Resources[i], rgb.r, rgb.g, rgb.b);
		} else {
			printf("XTerm*color%u: " FORMAT_X "\n", i, rgb.r, rgb.g, rgb.b);
		}
	}
}

static const char *Mintty[SchemeLen] = {
	"Black", "Red", "Green", "Yellow",
	"Blue", "Magenta", "Cyan", "White",
	"BoldBlack", "BoldRed", "BoldGreen", "BoldYellow",
	"BoldBlue", "BoldMagenta", "BoldCyan", "BoldWhite",
	[Background] = "BackgroundColour",
	[Foreground] = "ForegroundColour",
	[Cursor]     = "CursorColour",
};

static void outputMintty(const struct HSV *hsv, uint len) {
	for (uint i = 0; i < len; ++i) {
		if (!Mintty[i]) continue;
		struct RGB rgb = convert(hsv[i]);
		printf("%s=%hhu,%hhu,%hhu\n", Mintty[i], rgb.r, rgb.g, rgb.b);
	}
}

static void outputCSS(const struct HSV *hsv, uint len) {
	printf(":root {\n");
	for (uint i = 0; i < len; ++i) {
		struct RGB rgb = convert(hsv[i]);
		printf("\t--ansi%u: #" FORMAT_RGB ";\n", i, rgb.r, rgb.g, rgb.b);
	}
	printf("}\n");
	for (uint i = 0; i < len; ++i) {
		printf(
			".fg%u { color: var(--ansi%u); }\n"
			".bg%u { background-color: var(--ansi%u); }\n",
			i, i, i, i
		);
	}
}

enum {
	SwatchWidth = 64,
	SwatchHeight = 64,
	SwatchCols = 8,
};

static void outputPNG(const struct HSV *hsv, uint len) {
	uint rows = (len + SwatchCols - 1) / SwatchCols;
	uint width = SwatchWidth * SwatchCols;
	uint height = SwatchHeight * rows;
	pngHead(stdout, width, height, 8, PNGIndexed);

	struct RGB pal[len];
	for (uint i = 0; i < len; ++i) {
		pal[i] = convert(hsv[i]);
	}
	pngPalette(stdout, (byte *)pal, sizeof(pal));

	byte data[height][1 + width];
	memset(data, 0, sizeof(data));
	for (uint y = 0; y < height; ++y) {
		data[y][0] = (y % SwatchHeight ? PNGUp : PNGSub);
	}
	for (uint i = 0; i < len; ++i) {
		uint y = SwatchHeight * (i / SwatchCols);
		uint x = SwatchWidth * (i % SwatchCols);
		data[y][1 + x] = (x ? 1 : i);
	}
	pngData(stdout, (byte *)data, sizeof(data));
	pngTail(stdout);
}

int main(int argc, char *argv[]) {
	generate();

	OutputFn *output = outputRGB;
	const struct HSV *hsv = scheme;
	uint len = 16;

	int opt;
	while (0 < (opt = getopt(argc, argv, "Xacghilmp:stx"))) {
		switch (opt) {
			break; case 'X': output = outputXTerm;
			break; case 'a': len = 16;
			break; case 'c': output = outputEnum;
			break; case 'g': output = outputPNG;
			break; case 'h': output = outputHSV;
			break; case 'i': invert();
			break; case 'l': output = outputLinux;
			break; case 'm': output = outputMintty;
			break; case 'p': {
				uint p = strtoul(optarg, NULL, 0);
				if (p >= SchemeLen) return 1;
				hsv = &scheme[p];
				len = 1;
			}
			break; case 's': output = outputCSS;
			break; case 't': len = SchemeLen;
			break; case 'x': output = outputRGB;
			break; default:  return 1;
		}
	}

	output(hsv, len);
}
