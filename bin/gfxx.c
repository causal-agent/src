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
static bool flip;
static bool mirror;
static size_t scale = 1;

static size_t size = 1024 * 1024; // max from pipe.
static uint8_t *data;

extern int init(int argc, char *argv[]) {
    const char *path = NULL;
    const char *palPath = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "c:p:b:ern:fmw:z:"))) {
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
            case 'f': flip    ^= true; break;
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
        "gfxx -c %c -b %hhu %s-n %#zx -w %zu %s%s-z %zu\n",
        "gpr"[space],
        bits,
        endian ? "-e " : "",
        offset,
        width,
        flip ? "-f " : "",
        mirror ? "-m " : "",
        scale
    );
}

struct Iter {
    uint32_t *buf;
    size_t xres;
    size_t yres;
    size_t left;
    size_t x;
    size_t y;
};

static struct Iter iter(uint32_t *buf, size_t xres, size_t yres) {
    struct Iter it = { .buf = buf, .xres = xres, .yres = yres };
    if (mirror) it.x = width - 1;
    if (flip) it.y = yres / scale - 1;
    return it;
}

static bool nextX(struct Iter *it) {
    if (mirror) {
        if (it->x == it->left) return false;
        it->x--;
    } else {
        it->x++;
        if (it->x == it->left + width) return false;
    }
    return true;
}

static bool nextY(struct Iter *it) {
    if (flip) {
        if (it->y == 0) {
            it->left += width;
            it->y = it->yres / scale;
        }
        it->y--;
    } else {
        it->y++;
        if (it->y == it->yres / scale) {
            it->left += width;
            it->y = 0;
        }
    }
    it->x = it->left;
    if (mirror) it->x += width - 1;
    return (it->left < it->xres);
}

static bool next(struct Iter *it) {
    return (nextX(it) ? true : nextY(it));
}

static void put(const struct Iter *it, uint32_t p) {
    size_t scaledX = it->x * scale;
    size_t scaledY = it->y * scale;
    for (size_t fillY = scaledY; fillY < scaledY + scale; ++fillY) {
        if (fillY >= it->yres) break;
        for (size_t fillX = scaledX; fillX < scaledX + scale; ++fillX) {
            if (fillX >= it->xres) break;
            it->buf[fillY * it->xres + fillX] = p;
        }
    }
}

static void drawBits(struct Iter *it) {
    for (size_t i = offset; i < size; ++i) {
        for (int s = 0; s < 8; s += bits) {
            uint8_t n = data[i] >> (endian ? 8 - bits - s : s) & MASK(bits);
            if (space == COLOR_PALETTE) {
                put(it, palette[n]);
            } else if (space == COLOR_RGB && bits == 4) {
                put(it, RGB(SCALE(1, n & 1), SCALE(1, n & 2), SCALE(1, n & 4)));
            } else {
                put(it, GRAY(SCALE(bits, n)));
            }
            if (!next(it)) return;
        }
    }
}

static void draw8(struct Iter *it) {
    for (size_t i = offset; i < size; ++i) {
        if (space == COLOR_GRAYSCALE) {
            put(it, GRAY(data[i]));
        } else if (space == COLOR_PALETTE) {
            put(it, palette[data[i]]);
        } else {
            uint32_t r = (endian ? data[i] >> 5 : data[i] >> 0) & MASK(3);
            uint32_t g = (endian ? data[i] >> 2 : data[i] >> 3) & MASK(3);
            uint32_t b = (endian ? data[i] >> 0 : data[i] >> 6) & MASK(2);
            put(it, RGB(SCALE(3, r), SCALE(3, g), SCALE(2, b)));
        }
        if (!next(it)) break;
    }
}

static void draw16(struct Iter *it) {
    for (size_t i = offset; i + 1 < size; i += 2) {
        uint16_t n = (endian)
            ? (uint16_t)data[i+0] << 8 | (uint16_t)data[i+1]
            : (uint16_t)data[i+1] << 8 | (uint16_t)data[i+0];
        uint32_t r = n >> 11 & MASK(5);
        uint32_t g = n >>  5 & MASK(6);
        uint32_t b = n >>  0 & MASK(5);
        put(it, RGB(SCALE(5, r), SCALE(6, g), SCALE(5, b)));
        if (!next(it)) break;
    }
}

static void draw24(struct Iter *it) {
    for (size_t i = offset; i + 2 < size; i += 3) {
        if (endian) {
            put(it, RGB(data[i + 0], data[i + 1], data[i + 2]));
        } else {
            put(it, RGB(data[i + 2], data[i + 1], data[i + 0]));
        }
        if (!next(it)) break;
    }
}

static void draw32(struct Iter *it) {
    for (size_t i = offset; i + 3 < size; i += 4) {
        if (endian) {
            put(it, RGB(data[i + 1], data[i + 2], data[i + 3]));
        } else {
            put(it, RGB(data[i + 2], data[i + 1], data[i + 0]));
        }
        if (!next(it)) break;
    }
}

extern void draw(uint32_t *buf, size_t xres, size_t yres) {
    memset(buf, 0, 4 * xres * yres);
    struct Iter it = iter(buf, xres, yres);
    switch (bits) {
        case 8:  draw8(&it);  break;
        case 16: draw16(&it); break;
        case 24: draw24(&it); break;
        case 32: draw32(&it); break;
        default: drawBits(&it);
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
        break; case 'f': flip ^= true;
        break; case 'm': mirror ^= true;
        break; case '+': scale++;
        break; case '-': if (scale > 1) scale--;
    }
}
