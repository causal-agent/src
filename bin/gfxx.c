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
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

#define RGB(r,g,b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define GRAY(n)    RGB(n, n, n)
#define MASK(b)    ((1 << (b)) - 1)

static enum {
    COLOR_INDEXED,
    COLOR_GRAYSCALE,
    COLOR_RGB,
    COLOR__MAX,
} space = COLOR_RGB;
static uint32_t palette[256];
static enum {
    ENDIAN_LITTLE,
    ENDIAN_BIG,
} byteOrder, bitOrder;
static uint8_t bits[4] = { 8, 8, 8, 8 };
#define BITS_COLOR (bits[1] + bits[2] + bits[3])
#define BITS_TOTAL (bits[0] + BITS_COLOR)

static size_t offset;
static size_t width = 16;
static bool flip;
static bool mirror;
static size_t scale = 1;

static size_t size;
static uint8_t *data;

extern int init(int argc, char *argv[]) {
    const char *pal = NULL;
    const char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "c:p:b:e:E:n:fmw:z:"))) {
        switch (opt) {
            case 'c': switch (optarg[0]) {
                case 'i': space = COLOR_INDEXED; break;
                case 'g': space = COLOR_GRAYSCALE; break;
                case 'r': space = COLOR_RGB; break;
                default: return EX_USAGE;
            } break;
            case 'p': pal = optarg; break;
            case 'e': switch (optarg[0]) {
                case 'l': byteOrder = ENDIAN_LITTLE; break;
                case 'b': byteOrder = ENDIAN_BIG; break;
                default: return EX_USAGE;
            } break;
            case 'E': switch (optarg[0]) {
                case 'l': bitOrder = ENDIAN_LITTLE; break;
                case 'b': bitOrder = ENDIAN_BIG; break;
                default: return EX_USAGE;
            } break;
            case 'b': {
                size_t len = strlen(optarg);
                if (len < 3 || len > 4) return EX_USAGE;
                bits[0] = (len > 3) ? optarg[0] - '0' : 0;
                bits[1] = optarg[len-3] - '0';
                bits[2] = optarg[len-2] - '0';
                bits[3] = optarg[len-1] - '0';
            } break;
            case 'n': offset  = strtoul(optarg, NULL, 0); break;
            case 'f': flip   ^= true; break;
            case 'm': mirror ^= true; break;
            case 'w': width   = strtoul(optarg, NULL, 0); break;
            case 'z': scale   = strtoul(optarg, NULL, 0); break;
            default: return EX_USAGE;
        }
    }
    if (argc > optind) path = argv[optind];
    if (!width || !scale) return EX_USAGE;

    if (pal) {
        FILE *file = fopen(pal, "r");
        if (!file) err(EX_NOINPUT, "%s", pal);
        fread(palette, 4, 256, file);
        if (ferror(file)) err(EX_IOERR, "%s", pal);
        int error = fclose(file);
        if (error) err(EX_IOERR, "%s", pal);
    }

    if (path) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) err(EX_NOINPUT, "%s", path);

        struct stat stat;
        int error = fstat(fd, &stat);
        if (error) err(EX_IOERR, "%s", path);
        size = stat.st_size;

        data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) err(EX_IOERR, "%s", path);

    } else {
        size = 1024 * 1024;
        data = malloc(size);
        if (!data) err(EX_OSERR, "malloc(%zu)", size);
        size = fread(data, 1, size, stdin);
        if (ferror(stdin)) err(EX_IOERR, "stdin");
    }

    return EX_OK;
}

static char options[128];

static void formatOptions(void) {
    snprintf(
        options,
        sizeof(options),
        "gfxx -c %s -e%c -E%c -b %c%c%c%c -n %#zx %s%s-w %zu -z %zu",
        (const char *[COLOR__MAX]){ "indexed", "grayscale", "rgb" }[space],
        "lb"[byteOrder],
        "lb"[bitOrder],
        bits[0] + '0', bits[1] + '0', bits[2] + '0', bits[3] + '0',
        offset,
        flip ? "-f " : "",
        mirror ? "-m " : "",
        width,
        scale
    );
}

static void formatName(const char *ext) {
    snprintf(
        options,
        sizeof(options),
        "%c%c%c%c%c%c%c-%08zx-%c%c%02zu-%zu.%s",
        "igr"[space],
        "lb"[byteOrder],
        "lb"[bitOrder],
        bits[0] + '0', bits[1] + '0', bits[2] + '0', bits[3] + '0',
        offset,
        "xf"[flip],
        "xm"[mirror],
        width,
        scale,
        ext
    );
}

extern const char *title(void) {
    formatOptions();
    return options;
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
    return (it->left < it->xres / scale);
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

static uint8_t interp(uint8_t b, uint32_t n) {
    if (b == 8) return n;
    if (b == 0) return 0;
    return n * MASK(8) / MASK(b);
}

static uint32_t interpolate(uint32_t n) {
    uint32_t r, g, b;
    if (bitOrder == ENDIAN_LITTLE) {
        b = n & MASK(bits[3]);
        g = (n >>= bits[3]) & MASK(bits[2]);
        r = (n >>= bits[2]) & MASK(bits[1]);
    } else {
        r = n & MASK(bits[1]);
        g = (n >>= bits[1]) & MASK(bits[2]);
        b = (n >>= bits[2]) & MASK(bits[3]);
    }
    return RGB(interp(bits[1], r), interp(bits[2], g), interp(bits[3], b));
}

static void drawBits(struct Iter *it) {
    for (size_t i = offset; i < size; ++i) {
        for (uint8_t b = 0; b < 8; b += BITS_TOTAL) {
            uint8_t n;
            if (byteOrder == ENDIAN_BIG) {
                n = data[i] >> (8 - BITS_TOTAL - b) & MASK(BITS_TOTAL);
            } else {
                n = data[i] >> b & MASK(BITS_TOTAL);
            }
            if (space == COLOR_INDEXED) {
                put(it, palette[n]);
            } else if (space == COLOR_GRAYSCALE) {
                put(it, GRAY(interp(BITS_COLOR, n & MASK(BITS_COLOR))));
            } else if (space == COLOR_RGB) {
                put(it, interpolate(n));
            }
            if (!next(it)) return;
        }
    }
}

static void drawBytes(struct Iter *it) {
    uint8_t bytes = (BITS_TOTAL + 7) / 8;
    for (size_t i = offset; i < size; i += bytes) {
        uint32_t n = 0;
        for (size_t b = 0; b < bytes; ++b) {
            n <<= 8;
            n |= (byteOrder == ENDIAN_BIG) ? data[i+b] : data[i+bytes-b-1];
        }
        if (space == COLOR_INDEXED) {
            put(it, palette[n & 0xFF]);
        } else if (space == COLOR_GRAYSCALE) {
            put(it, GRAY(interp(BITS_COLOR, n & MASK(BITS_COLOR))));
        } else if (space == COLOR_RGB) {
            put(it, interpolate(n));
        }
        if (!next(it)) return;
    }
}

extern void draw(uint32_t *buf, size_t xres, size_t yres) {
    memset(buf, 0, 4 * xres * yres);
    struct Iter it = iter(buf, xres, yres);
    if (BITS_TOTAL >= 8) {
        drawBytes(&it);
    } else {
        drawBits(&it);
    }
}

static void palSample(void) {
    size_t temp = scale;
    scale = 1;
    draw(palette, 256, 1);
    scale = temp;
}

static void palDump(void) {
    formatName("dat");
    FILE *file = fopen(options, "w");
    if (!file) { warn("%s", options); return; }
    size_t count = fwrite(palette, 4, 256, file);
    if (count != 256) { warn("%s", options); return; }
    int error = fclose(file);
    if (error) { warn("%s", options); return; }
}

static const uint8_t PRESETS[][4] = {
    { 0, 0, 1, 0 },
    { 0, 1, 1, 0 },
    { 1, 1, 1, 1 },
    { 2, 2, 2, 2 },
    { 0, 3, 3, 2 },
    { 1, 5, 5, 5 },
    { 0, 5, 6, 5 },
    { 0, 8, 8, 8 },
    { 8, 8, 8, 8 },
};
#define PRESETS_LEN (sizeof(PRESETS) / sizeof(PRESETS[0]))
static uint8_t preset = PRESETS_LEN - 1;
static void setPreset(void) {
    for (int i = 0; i < 4; ++i) {
        bits[i] = PRESETS[preset][i];
    }
}

extern void input(char in) {
    size_t pixel = (BITS_TOTAL + 7) / 8;
    size_t row = width * pixel;
    switch (in) {
        case 'q': formatOptions(); printf("%s\n", options); exit(EX_OK);
        break; case 'o': formatOptions(); printf("%s\n", options);
        break; case '[': if (!space--) space = COLOR__MAX - 1;
        break; case ']': if (++space == COLOR__MAX) space = 0;
        break; case 'p': palSample();
        break; case 'P': palDump();
        break; case '{': if (!preset--) preset = PRESETS_LEN - 1; setPreset();
        break; case '}': if (++preset == PRESETS_LEN) preset = 0; setPreset();
        break; case 'e': byteOrder ^= ENDIAN_BIG;
        break; case 'E': bitOrder ^= ENDIAN_BIG;
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
