/* Copyright (c) 2018, June McEnroe <programble@gmail.com>
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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

#define RGB(r,g,b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define GRAY(n)    RGB(n, n, n)
#define MASK(b)    ((1 << b) - 1)
#define SCALE(b,n) (uint8_t)(255 * (uint32_t)(n) / MASK(b))

static enum {
    COLOR_GRAYSCALE,
    COLOR_PALETTE,
    COLOR_RGB,
    COLOR__MAX,
} space;
static uint8_t bits = 1;
static bool endian;
static uint32_t palette[256] = {
#define X(...) __VA_ARGS__, __VA_ARGS__
    X(X(X(X(X(X(X(0x000000, 0xFFFFFF)))))))
#undef X
};

static size_t offset;

static size_t width = 16;
static bool mirror;
static size_t scale = 1;

static size_t size = 1024 * 1024; // max from pipe.
static uint8_t *data;

extern int init(int argc, char *argv[]) {
    const char *path = NULL;
    const char *palPath = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "c:p:b:ern:mw:z:"))) {
        switch (opt) {
            case 'c': switch (optarg[0]) {
                case 'g': space = COLOR_GRAYSCALE; break;
                case 'p': space = COLOR_PALETTE; break;
                case 'r': space = COLOR_RGB; break;
                default: return EX_USAGE;
            } break;
            case 'p': palPath  = optarg; break;
            case 'b': bits     = strtoul(optarg, NULL, 0); break;
            case 'e': endian  ^= true; break;
            case 'n': offset   = strtoul(optarg, NULL, 0); break;
            case 'w': width    = strtoul(optarg, NULL, 0); break;
            case 'm': mirror  ^= true; break;
            case 'z': scale    = strtoul(optarg, NULL, 0); break;
            default: return EX_USAGE;
        }
    }
    if (argc > optind) path = argv[optind];
    if (!bits || !width || !scale) return EX_USAGE;

    if (palPath) {
        FILE *pal = fopen(palPath, "r");
        if (!pal) err(EX_NOINPUT, "%s", palPath);

        fread(palette, 1, sizeof(palette), pal);
        if (ferror(pal)) err(EX_IOERR, "%s", palPath);

        fclose(pal);
    }

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) err(EX_NOINPUT, "%s", path);

    if (path) {
        struct stat stat;
        int error = fstat(fileno(file), &stat);
        if (error) err(EX_IOERR, "%s", path);
        size = stat.st_size;
    }

    data = malloc(size);
    if (!data) err(EX_OSERR, "malloc(%zu)", size);

    size = fread(data, 1, size, file);
    if (ferror(file)) err(EX_IOERR, "%s", path);

    fclose(file);

    return EX_OK;
}

static void printOpts(void) {
    printf(
        "gfxx -c %c -b %hhu %s-n %#zx -w %zu %s-z %zu\n",
        "gpr"[space],
        bits,
        endian ? "-e " : "",
        offset,
        width,
        mirror ? "-m " : "",
        scale
    );
}

struct Pos {
    uint32_t *buf;
    size_t xres;
    size_t yres;
    size_t left;
    size_t x;
    size_t y;
};

static bool next(struct Pos *pos) {
    if (mirror) {
        if (pos->x == pos->left) {
            pos->y++;
            pos->x = pos->left + width;
        }
        if (pos->y == pos->yres / scale) {
            pos->left += width;
            pos->x = pos->left + width;
            pos->y = 0;
        }
        pos->x--;
    } else {
        pos->x++;
        if (pos->x - pos->left == width) {
            pos->y++;
            pos->x = pos->left;
        }
        if (pos->y == pos->yres / scale) {
            pos->left += width;
            pos->x = pos->left;
            pos->y = 0;
        }
    }
    return (pos->left < pos->xres);
}

static void put(const struct Pos *pos, uint32_t p) {
    size_t scaledX = pos->x * scale;
    size_t scaledY = pos->y * scale;
    for (size_t fillY = scaledY; fillY < scaledY + scale; ++fillY) {
        if (fillY >= pos->yres) break;
        for (size_t fillX = scaledX; fillX < scaledX + scale; ++fillX) {
            if (fillX >= pos->xres) break;
            pos->buf[fillY * pos->xres + fillX] = p;
        }
    }
}

static void drawBits(struct Pos *pos) {
    for (size_t i = offset; i < size; ++i) {
        for (int s = 0; s < 8; s += bits) {
            uint8_t n = data[i] >> (endian ? 8 - bits - s : s) & MASK(bits);
            if (space == COLOR_PALETTE) {
                put(pos, palette[n]);
            } else if (space == COLOR_RGB && bits == 4) {
                put(pos, RGB(SCALE(1, n & 1), SCALE(1, n & 2), SCALE(1, n & 4)));
            } else {
                put(pos, GRAY(SCALE(bits, n)));
            }
            if (!next(pos)) return;
        }
    }
}

static void draw8(struct Pos *pos) {
    for (size_t i = offset; i < size; ++i) {
        if (space == COLOR_GRAYSCALE) {
            put(pos, GRAY(data[i]));
        } else if (space == COLOR_PALETTE) {
            put(pos, palette[data[i]]);
        } else {
            uint32_t r = (endian ? data[i] >> 5 : data[i] >> 0) & MASK(3);
            uint32_t g = (endian ? data[i] >> 2 : data[i] >> 3) & MASK(3);
            uint32_t b = (endian ? data[i] >> 0 : data[i] >> 6) & MASK(2);
            put(pos, RGB(SCALE(3, r), SCALE(3, g), SCALE(2, b)));
        }
        if (!next(pos)) break;
    }
}

static void draw16(struct Pos *pos) {
    for (size_t i = offset; i + 1 < size; i += 2) {
        uint16_t n = (endian)
            ? (uint16_t)data[i+0] << 8 | (uint16_t)data[i+1]
            : (uint16_t)data[i+1] << 8 | (uint16_t)data[i+0];
        uint32_t r = n >> 11 & MASK(5);
        uint32_t g = n >>  5 & MASK(6);
        uint32_t b = n >>  0 & MASK(5);
        put(pos, RGB(SCALE(5, r), SCALE(6, g), SCALE(5, b)));
        if (!next(pos)) break;
    }
}

static void draw24(struct Pos *pos) {
    for (size_t i = offset; i + 2 < size; i += 3) {
        if (endian) {
            put(pos, RGB(data[i + 0], data[i + 1], data[i + 2]));
        } else {
            put(pos, RGB(data[i + 2], data[i + 1], data[i + 0]));
        }
        if (!next(pos)) break;
    }
}

static void draw32(struct Pos *pos) {
    for (size_t i = offset; i + 3 < size; i += 4) {
        if (endian) {
            put(pos, RGB(data[i + 1], data[i + 2], data[i + 3]));
        } else {
            put(pos, RGB(data[i + 2], data[i + 1], data[i + 0]));
        }
        if (!next(pos)) break;
    }
}

extern void draw(uint32_t *buf, size_t xres, size_t yres) {
    memset(buf, 0, 4 * xres * yres);
    struct Pos pos = {
        .buf = buf,
        .xres = xres,
        .yres = yres,
        .x = (mirror) ? width - 1 : 0
    };
    switch (bits) {
        case 8:  draw8(&pos);  break;
        case 16: draw16(&pos); break;
        case 24: draw24(&pos); break;
        case 32: draw32(&pos); break;
        default: drawBits(&pos);
    }
}

static void samplePalette(void) {
    size_t temp = scale;
    scale = 1;
    draw(palette, 256, 1);
    scale = temp;
}

extern void input(char in) {
    size_t pixel = (bits + 7) / 8;
    size_t row = width * bits / 8;
    switch (in) {
        case 'q': printOpts(); exit(EX_OK);
        break; case 'o': printOpts();
        break; case '[': if (!space--) space = COLOR__MAX - 1;
        break; case ']': if (++space == COLOR__MAX) space = 0;
        break; case 'p': samplePalette();
        break; case '{': if (bits > 16) bits -= 8; else bits = (bits + 1) / 2;
        break; case '}': if (bits < 16) bits *= 2; else if (bits < 32) bits += 8;
        break; case 'e': endian ^= true;
        break; case 'h': if (offset) offset--;
        break; case 'j': offset += pixel;
        break; case 'k': if (offset >= pixel) offset -= pixel;
        break; case 'l': offset++;
        break; case 'H': if (offset >= row) offset -= row;
        break; case 'J': offset += width * row;
        break; case 'K': if (offset >= width * row) offset -= width * row;
        break; case 'L': offset += row;
        break; case '.': width++;
        break; case ',': if (width > 1) width--;
        break; case '>': width *= 2;
        break; case '<': if (width / 2 >= 1) width /= 2;
        break; case 'm': mirror ^= true;
        break; case '+': scale++;
        break; case '-': if (scale > 1) scale--;
    }
}
