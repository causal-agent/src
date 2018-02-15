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

#include <arpa/inet.h>
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
#include <zlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MASK(b) ((1 << (b)) - 1)

#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define GRAY(n) RGB(n, n, n)

static enum {
    COLOR_INDEXED,
    COLOR_GRAYSCALE,
    COLOR_RGB,
    COLOR__MAX,
} space = COLOR_RGB;
static const char *COLOR__STR[COLOR__MAX] = { "indexed", "grayscale", "rgb" };
static uint32_t palette[256];

static enum {
    ENDIAN_LITTLE,
    ENDIAN_BIG,
} byteOrder, bitOrder;

enum { PAD, R, G, B };
static uint8_t bits[4] = { 8, 8, 8, 8 };
#define BITS_COLOR (bits[R] + bits[G] + bits[B])
#define BITS_TOTAL (bits[PAD] + BITS_COLOR)

static size_t offset;
static size_t width = 16;
static bool flip;
static bool mirror;
static size_t scale = 1;

static const char *prefix = "gfxx";

static size_t size;
static uint8_t *data;

int init(int argc, char *argv[]) {
    const char *pal = NULL;
    const char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "c:p:b:e:E:n:fmw:z:o:"))) {
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
                if (strlen(optarg) < 4) return EX_USAGE;
                for (int i = 0; i < 4; ++i) {
                    bits[i] = optarg[i] - '0';
                }
            } break;
            case 'n': offset  = strtoul(optarg, NULL, 0); break;
            case 'f': flip   ^= true; break;
            case 'm': mirror ^= true; break;
            case 'w': width   = strtoul(optarg, NULL, 0); break;
            case 'z': scale   = strtoul(optarg, NULL, 0); break;
            case 'o': prefix  = optarg; break;
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
        COLOR__STR[space],
        "lb"[byteOrder],
        "lb"[bitOrder],
        bits[PAD] + '0', bits[R] + '0', bits[G] + '0', bits[B] + '0',
        offset,
        flip ? "-f " : "",
        mirror ? "-m " : "",
        width,
        scale
    );
}

const char *status(void) {
    formatOptions();
    return options;
}

struct Iter {
    uint32_t *buf;
    size_t bufWidth;
    size_t bufHeight;
    size_t left;
    size_t x;
    size_t y;
};

static struct Iter iter(uint32_t *buf, size_t bufWidth, size_t bufHeight) {
    struct Iter it = { .buf = buf, .bufWidth = bufWidth, .bufHeight = bufHeight };
    if (mirror) it.x = width - 1;
    if (flip) it.y = bufHeight / scale - 1;
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
            it->y = it->bufHeight / scale;
        }
        it->y--;
    } else {
        it->y++;
        if (it->y == it->bufHeight / scale) {
            it->left += width;
            it->y = 0;
        }
    }
    it->x = it->left;
    if (mirror) it->x += width - 1;
    return (it->left < it->bufWidth / scale);
}

static bool next(struct Iter *it) {
    return nextX(it) || nextY(it);
}

static void put(const struct Iter *it, uint32_t pixel) {
    size_t scaledX = it->x * scale;
    size_t scaledY = it->y * scale;
    for (size_t fillY = scaledY; fillY < scaledY + scale; ++fillY) {
        if (fillY >= it->bufHeight) break;
        for (size_t fillX = scaledX; fillX < scaledX + scale; ++fillX) {
            if (fillX >= it->bufWidth) break;
            it->buf[fillY * it->bufWidth + fillX] = pixel;
        }
    }
}

static uint8_t interp(uint8_t b, uint32_t n) {
    if (b == 8) return n;
    if (b == 0) return 0;
    return n * MASK(8) / MASK(b);
}

static uint32_t interpolate(uint32_t rgb) {
    uint32_t r, g, b;
    if (bitOrder == ENDIAN_LITTLE) {
        b = rgb & MASK(bits[B]);
        g = (rgb >>= bits[B]) & MASK(bits[G]);
        r = (rgb >>= bits[G]) & MASK(bits[R]);
    } else {
        r = rgb & MASK(bits[R]);
        g = (rgb >>= bits[R]) & MASK(bits[G]);
        b = (rgb >>= bits[G]) & MASK(bits[B]);
    }
    return RGB(interp(bits[R], r), interp(bits[G], g), interp(bits[B], b));
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
    for (size_t i = offset; i + bytes <= size; i += bytes) {
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

static unsigned counter = 1;
static char pngPath[FILENAME_MAX];
static FILE *png;

static uint32_t crc;
static void pngWrite(const void *data, size_t size) {
    crc = crc32(crc, data, size);
    size_t count = fwrite(data, 1, size, png);
    if (count < size) err(EX_IOERR, "%s", pngPath);
}
static void pngUint32(uint32_t data) {
    uint32_t net = htonl(data);
    pngWrite(&net, 4);
}

#define ONCE(before, after) for (int _once = ((before), 1); _once; (after), --_once)
#define PNG_CHUNK(type, size) ONCE( \
    (pngUint32((size)), crc = crc32(0, Z_NULL, 0), pngWrite((type), 4)), \
    pngUint32(crc) \
)

static void pngDump(uint32_t *src, size_t srcWidth, size_t srcHeight) {
    int error;

    size_t scanline = 1 + (space == COLOR_GRAYSCALE ? 1 : 3) * srcWidth;
    uint8_t filt[scanline * srcHeight];
    for (size_t y = 0; y < srcHeight; ++y) {
        filt[y * scanline] = 0; // None
        for (size_t x = 0; x < srcWidth; ++x) {
            if (space == COLOR_GRAYSCALE) {
                filt[y * scanline + 1 + x] = src[y * srcWidth + x];
            } else {
                uint8_t *filtPixel = &filt[y * scanline + 1 + 3 * x];
                filtPixel[0] = src[y * srcWidth + x] >> 16;
                filtPixel[1] = src[y * srcWidth + x] >> 8;
                filtPixel[2] = src[y * srcWidth + x];
            }
        }
    }

    size_t dataSize = compressBound(sizeof(filt));
    uint8_t data[dataSize];
    error = compress(data, &dataSize, filt, sizeof(filt));
    if (error != Z_OK) errx(EX_SOFTWARE, "compress: %d", error);

    snprintf(pngPath, sizeof(pngPath), "%s%04u.png", prefix, counter++);
    png = fopen(pngPath, "wx");
    if (!png) { warn("%s", pngPath); return; }
    printf("%s\n", pngPath);

    uint8_t signature[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    uint8_t header[] = { 8, (space == COLOR_GRAYSCALE ? 0 : 2), 0, 0, 0 };
    char software[] = "Software";
    formatOptions();

    pngWrite(signature, sizeof(signature));
    PNG_CHUNK("IHDR", 4 + 4 + sizeof(header)) {
        pngUint32(srcWidth);
        pngUint32(srcHeight);
        pngWrite(header, sizeof(header));
    }
    PNG_CHUNK("tEXt", sizeof(software) + strlen(options)) {
        pngWrite(software, sizeof(software));
        pngWrite(options, strlen(options));
    }
    if (space == COLOR_GRAYSCALE) {
        PNG_CHUNK("sBIT", 1) {
            uint8_t sbit = BITS_COLOR;
            pngWrite(&sbit, 1);
        }
    } else {
        PNG_CHUNK("sBIT", 3) {
            uint8_t sbit[] = { MAX(bits[R], 1), MAX(bits[G], 1), MAX(bits[B], 1) };
            pngWrite(sbit, sizeof(sbit));
        }
    }
    PNG_CHUNK("IDAT", dataSize) {
        pngWrite(data, dataSize);
    }
    PNG_CHUNK("IEND", 0);

    error = fclose(png);
    if (error) err(EX_IOERR, "%s", pngPath);
}

static enum {
    DUMP_NONE,
    DUMP_ONE,
    DUMP_ALL,
} dump;

void draw(uint32_t *buf, size_t bufWidth, size_t bufHeight) {
    memset(buf, 0, 4 * bufWidth * bufHeight);
    struct Iter it = iter(buf, bufWidth, bufHeight);
    if (BITS_TOTAL >= 8) {
        drawBytes(&it);
    } else {
        drawBits(&it);
    }
    if (dump) pngDump(buf, bufWidth, bufHeight);
    if (dump == DUMP_ONE) dump = DUMP_NONE;
}

static void palSample(void) {
    size_t temp = scale;
    scale = 1;
    draw(palette, 256, 1);
    scale = temp;
}

static void palDump(void) {
    size_t count = fwrite(palette, 4, 256, stdout);
    if (count < 256) err(EX_IOERR, "stdout");
}

static uint8_t bit = 0;
static void setBit(char in) {
    bits[bit++] = in - '0';
    bit &= 3;
}

static const uint8_t PRESETS[][4] = {
    { 0, 0, 1, 0 },
    { 0, 1, 1, 0 },
    { 1, 1, 1, 1 },
    { 2, 2, 2, 2 },
    { 0, 3, 3, 2 },
    { 4, 4, 4, 4 },
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
    bit = 0;
}

bool input(char in) {
    size_t pixel = (BITS_TOTAL + 7) / 8;
    size_t row = width * BITS_TOTAL / 8;
    switch (in) {
        case 'q': return false;
        break; case 'x': dump = DUMP_ONE;
        break; case 'X': dump ^= DUMP_ALL;
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
        break; case '<': if (width > 1) width /= 2;
        break; case 'f': flip ^= true;
        break; case 'm': mirror ^= true;
        break; case '+': scale++;
        break; case '-': if (scale > 1) scale--;
        break; default: if (in >= '0' && in <= '9') setBit(in);
    }
    return true;
}
