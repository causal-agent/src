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
#include <sysexits.h>

#include "gfx/gfx.h"

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

static double complex origin = -0.75 + 0.0 * I;
static double complex scale = 3.5 + 2.0 * I;
static double complex rotate = 1;

void draw(uint32_t *buf, size_t width, size_t height) {
    for (size_t y = 0; y < height; ++y) {
        double complex zy = ((double)y / (double)height - 0.5) * cimag(scale) * I;
        for (size_t x = 0; x < width; ++x) {
            double complex zx = ((double)x / (double)width - 0.5) * creal(scale);
            uint64_t n = mandelbrot((origin + zx + zy) * rotate);
            uint32_t g = (double)n / (double)depth * 255.0;
            buf[y * width + x] = g << 16 | g << 8 | g;
        }
    }
}

bool input(char in) {
    switch (in) {
        case 'q': return false;
        case '.': depth++; break;
        case ',': if (depth) depth--; break;
        case '+': scale *= 0.9; break;
        case '-': scale *= 1.1; break;
        case 'l': origin += creal(scale) * 0.01; break;
        case 'h': origin -= creal(scale) * 0.01; break;
        case 'j': origin += cimag(scale) * 0.01 * I; break;
        case 'k': origin -= cimag(scale) * 0.01 * I; break;
    }
    return true;
}

const char *status(void) {
    static char buf[256];
    snprintf(
        buf, sizeof(buf),
        "(%u) (%g + %gi) (%g + %gi)",
        depth,
        creal(origin), cimag(origin),
        creal(scale), cimag(scale)
    );
    return buf;
}

int init(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    return EX_OK;
}
