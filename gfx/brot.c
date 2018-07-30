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

#include <complex.h>
#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include "gfx.h"

#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define GRAY(n) RGB(n, n, n)

static double absSq(double complex z) {
	return creal(z) * creal(z) + cimag(z) * cimag(z);
}

static uint32_t depth = 50;

static uint32_t mandelbrot(double complex c) {
	double complex z = 0;
	for (uint32_t i = 0; i < depth; ++i) {
		if (absSq(z) > 4.0) return i;
		z = z * z + c;
	}
	return 0;
}

static double complex translate = -0.75;
static double complex transform = 2.5;

static uint32_t samples = 1;

static void sample(uint32_t *buf, size_t width, size_t height) {
	double yRatio = (height > width) ? (double)height / (double)width : 1.0;
	double xRatio = (width > height) ? (double)width / (double)height : 1.0;

	memset(buf, 0, 4 * width * height);
	size_t superWidth = width * samples;
	size_t superHeight = height * samples;
	for (size_t y = 0; y < superHeight; ++y) {
		for (size_t x = 0; x < superWidth; ++x) {
			double zx = (((double)x + 0.5) / (double)superWidth - 0.5) * xRatio;
			double zy = (((double)y + 0.5) / (double)superHeight - 0.5) * yRatio;
			uint32_t n = mandelbrot((zx + zy * I) * transform + translate);
			buf[(y / samples) * width + (x / samples)] += n;
		}
	}
}

static void color(uint32_t *buf, size_t width, size_t height) {
	for (size_t i = 0; i < width * height; ++i) {
		buf[i] = GRAY(255 * buf[i] / samples / samples / depth);
	}
}

static double frameTime;
void draw(uint32_t *buf, size_t width, size_t height) {
	clock_t t0 = clock();
	sample(buf, width, height);
	color(buf, width, height);
	frameTime = (double)(clock() - t0) / (double)CLOCKS_PER_SEC;
}

static double translateStep = 1.0 / 128.0;
static double rotateStep = 1.0 / 128.0;
static double scaleStep = 1.0 / 32.0;

bool input(char in) {
	const double PI = acos(-1.0);
	switch (in) {
		break; case 'q': return false;
		break; case '.': depth++;
		break; case ',': if (depth > 1) depth--;
		break; case 'l': translate += translateStep * transform;
		break; case 'h': translate -= translateStep * transform;
		break; case 'j': translate += translateStep * I * transform;
		break; case 'k': translate -= translateStep * I * transform;
		break; case 'u': transform *= cexp(rotateStep * PI * I);
		break; case 'i': transform /= cexp(rotateStep * PI * I);
		break; case '+': transform *= 1.0 - scaleStep;
		break; case '-': transform /= 1.0 - scaleStep;
		break; case '0': translate = -0.75; transform = 2.5;
		break; case ']': samples++;
		break; case '[': if (samples > 1) samples--;
	}
	return true;
}

const char *status(void) {
	static char buf[256];
	snprintf(
		buf, sizeof(buf),
		"brot -s %u -i %u -t %g%+gi -f %g%+gi # %.6f",
		samples, depth,
		creal(translate), cimag(translate),
		creal(transform), cimag(transform),
		frameTime
	);
	return buf;
}

static double complex parseComplex(const char *str) {
	double real = 0.0, imag = 0.0;
	real = strtod(str, (char **)&str);
	if (str[0] == 'i') {
		imag = real;
		real = 0.0;
	} else if (str[0]) {
		imag = strtod(str, NULL);
	}
	return real + imag * I;
}

int init(int argc, char *argv[]) {
	int opt;
	while (0 < (opt = getopt(argc, argv, "f:i:s:t:"))) {
		switch (opt) {
			break; case 'f': transform = parseComplex(optarg);
			break; case 'i': depth = strtoul(optarg, NULL, 0);
			break; case 's': samples = strtoul(optarg, NULL, 0);
			break; case 't': translate = parseComplex(optarg);
			break; default: return EX_USAGE;
		}
	}
	return EX_OK;
}
