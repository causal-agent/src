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

static void png(const struct Hsv *scheme, uint8_t len) {
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
        struct Rgb rgb = toRgb(scheme[i]);
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

static void hsv(const struct Hsv *scheme, uint8_t len) {
    for (uint8_t i = 0; i < len; ++i) {
        printf("%g,%g,%g\n", scheme[i].h, scheme[i].s, scheme[i].v);
    }
}

static void hex(const struct Hsv *scheme, uint8_t len) {
    for (uint8_t i = 0; i < len; ++i) {
        struct Rgb rgb = toRgb(scheme[i]);
        printf("%02x%02x%02x\n", rgb.r, rgb.g, rgb.b);
    }
}

static void linux(const struct Hsv *scheme, uint8_t len) {
    if (len > 16) len = 16;
    for (uint8_t i = 0; i < len; ++i) {
        struct Rgb rgb = toRgb(scheme[i]);
        printf("\x1B]P%x%02x%02x%02x", i, rgb.r, rgb.g, rgb.b);
    }
}

static const struct Hsv R = {   0.0, 1.0, 1.0 };
static const struct Hsv Y = {  60.0, 1.0, 1.0 };
static const struct Hsv G = { 120.0, 1.0, 1.0 };
static const struct Hsv C = { 180.0, 1.0, 1.0 };
static const struct Hsv B = { 240.0, 1.0, 1.0 };
static const struct Hsv M = { 300.0, 1.0, 1.0 };

static struct Hsv p(struct Hsv o, double hd, double sf, double vf) {
    return (struct Hsv) { o.h + hd, o.s * sf, o.v * vf };
}

enum { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
struct Ansi {
    struct Hsv dark[8];
    struct Hsv light[8];
};
struct Terminal {
    struct Ansi ansi;
    struct Hsv background, text, bold, selection, cursor;
};

#define HSV_LEN(x) (sizeof(x) / sizeof(struct Hsv))

static struct Ansi genAnsi(void) {
    struct Ansi ansi = {
        .light = {
            [BLACK]   = p(R, +45.0, 0.3, 0.3),
            [RED]     = p(R, +10.0, 0.9, 0.8),
            [GREEN]   = p(G, -55.0, 0.8, 0.6),
            [YELLOW]  = p(Y, -20.0, 0.8, 0.8),
            [BLUE]    = p(B, -55.0, 0.4, 0.5),
            [MAGENTA] = p(M, +45.0, 0.4, 0.6),
            [CYAN]    = p(C, -60.0, 0.3, 0.6),
            [WHITE]   = p(R, +45.0, 0.3, 0.8),
        },
    };
    ansi.dark[BLACK] = p(ansi.light[BLACK], 0.0, 1.0, 0.3);
    ansi.dark[WHITE] = p(ansi.light[WHITE], 0.0, 1.0, 0.6);
    for (int i = RED; i < WHITE; ++i) {
        ansi.dark[i] = p(ansi.light[i], 0.0, 1.0, 0.8);
    }
    return ansi;
}

static struct Terminal genTerminal(struct Ansi ansi) {
    return (struct Terminal) {
        .ansi       = ansi,
        .background = p(ansi.dark[BLACK],    0.0, 1.0, 0.9),
        .text       = p(ansi.light[WHITE],   0.0, 1.0, 0.9),
        .bold       = p(ansi.light[WHITE],   0.0, 1.0, 1.0),
        .selection  = p(ansi.light[RED],   +10.0, 1.0, 0.8),
        .cursor     = p(ansi.dark[WHITE],    0.0, 1.0, 0.8),
    };
}

int main(int argc, char *argv[]) {
    enum { ANSI, TERMINAL } generate = ANSI;
    enum { HSV, HEX, LINUX, PNG } output = HEX;

    int opt;
    while (0 < (opt = getopt(argc, argv, "aghltx"))) {
        switch (opt) {
            case 'a': generate = ANSI; break;
            case 'g': output = PNG; break;
            case 'h': output = HSV; break;
            case 'l': output = LINUX; break;
            case 't': generate = TERMINAL; break;
            case 'x': output = HEX; break;
            default: return EX_USAGE;
        }
    }

    struct Ansi ansi = genAnsi();
    struct Terminal terminal = genTerminal(ansi);

    const struct Hsv *scheme;
    uint8_t len;
    switch (generate) {
        case ANSI: {
            scheme = (struct Hsv *)&ansi;
            len = HSV_LEN(ansi);
        } break;
        case TERMINAL: {
            scheme = (struct Hsv *)&terminal;
            len = HSV_LEN(terminal);
        } break;
    }

    switch (output) {
        case HSV: hsv(scheme, len); break;
        case HEX: hex(scheme, len); break;
        case LINUX: linux(scheme, len); break;
        case PNG: png(scheme, len); break;
    }

    return EX_OK;
}
