/*
 * Copyright (C) 2018  June McEnroe <june@causal.agency>
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
#include <zlib.h>

struct Hsv {
    double h, s, v;
};

struct Rgb {
    uint8_t r, g, b;
};

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
enum { NONE, SUB, UP, AVERAGE, PAETH };

static void png(const struct Hsv *hsv, uint8_t len) {
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
    for (uint8_t i = 0; i < len; ++i) {
        struct Rgb rgb = toRgb(hsv[i]);
        pngWrite(&rgb, 3);
    }
    pngInt(crc);

    uint8_t data[height][1 + width];
    memset(data, 0, sizeof(data));
    for (uint32_t y = 0; y < height; ++y) {
        data[y][0] = (y % swatchHeight) ? UP : SUB;
    }
    for (uint8_t i = 0; i < len; ++i) {
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

int main() {
    struct Hsv scheme[16] = {
        { 0.0,   1.0, 1.0 },
        { 45.0,  1.0, 1.0 },
        { 90.0,  1.0, 1.0 },
        { 135.0, 1.0, 1.0 },
        { 180.0, 1.0, 1.0 },
        { 225.0, 1.0, 1.0 },
        { 270.0, 1.0, 1.0 },
        { 315.0, 1.0, 1.0 },
        { 0.0,   0.5, 1.0 },
        { 45.0,  0.5, 1.0 },
        { 90.0,  0.5, 1.0 },
        { 135.0, 0.5, 1.0 },
        { 180.0, 0.5, 1.0 },
        { 225.0, 0.5, 1.0 },
        { 270.0, 0.5, 1.0 },
        { 315.0, 0.5, 1.0 },
    };
    png(scheme, 16);
    return EX_OK;
}
