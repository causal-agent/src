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

#include "gfx/gfx.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MASK(b) ((1 << (b)) - 1)

#define RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define GRAY(n) RGB(n, n, n)

static enum {
    COLOR_INDEXED,
    COLOR_GRAYSCALE,
    COLOR_RGB,
    COLOR__COUNT,
} space = COLOR_RGB;
static const char *COLOR__STR[COLOR__COUNT] = { "indexed", "grayscale", "rgb" };

static uint32_t palette[256] = {
    0xFF0000, 0xFF0600, 0xFF0C00, 0xFF1200, 0xFF1800, 0xFF1E00, 0xFF2400, 0xFF2A00,
    0xFF3000, 0xFF3600, 0xFF3C00, 0xFF4200, 0xFF4800, 0xFF4E00, 0xFF5400, 0xFF5A00,
    0xFF6000, 0xFF6600, 0xFF6C00, 0xFF7200, 0xFF7800, 0xFF7E00, 0xFF8300, 0xFF8900,
    0xFF8F00, 0xFF9500, 0xFF9B00, 0xFFA100, 0xFFA700, 0xFFAD00, 0xFFB300, 0xFFB900,
    0xFFBF00, 0xFFC500, 0xFFCB00, 0xFFD100, 0xFFD700, 0xFFDD00, 0xFFE300, 0xFFE900,
    0xFFEF00, 0xFFF500, 0xFFFB00, 0xFDFF00, 0xF7FF00, 0xF1FF00, 0xEBFF00, 0xE5FF00,
    0xDFFF00, 0xD9FF00, 0xD3FF00, 0xCDFF00, 0xC7FF00, 0xC1FF00, 0xBBFF00, 0xB5FF00,
    0xAFFF00, 0xA9FF00, 0xA3FF00, 0x9DFF00, 0x97FF00, 0x91FF00, 0x8BFF00, 0x85FF00,
    0x80FF00, 0x7AFF00, 0x74FF00, 0x6EFF00, 0x68FF00, 0x62FF00, 0x5CFF00, 0x56FF00,
    0x50FF00, 0x4AFF00, 0x44FF00, 0x3EFF00, 0x38FF00, 0x32FF00, 0x2CFF00, 0x26FF00,
    0x20FF00, 0x1AFF00, 0x14FF00, 0x0EFF00, 0x08FF00, 0x02FF00, 0x00FF04, 0x00FF0A,
    0x00FF10, 0x00FF16, 0x00FF1C, 0x00FF22, 0x00FF28, 0x00FF2E, 0x00FF34, 0x00FF3A,
    0x00FF40, 0x00FF46, 0x00FF4C, 0x00FF52, 0x00FF58, 0x00FF5E, 0x00FF64, 0x00FF6A,
    0x00FF70, 0x00FF76, 0x00FF7C, 0x00FF81, 0x00FF87, 0x00FF8D, 0x00FF93, 0x00FF99,
    0x00FF9F, 0x00FFA5, 0x00FFAB, 0x00FFB1, 0x00FFB7, 0x00FFBD, 0x00FFC3, 0x00FFC9,
    0x00FFCF, 0x00FFD5, 0x00FFDB, 0x00FFE1, 0x00FFE7, 0x00FFED, 0x00FFF3, 0x00FFF9,
    0x00FFFF, 0x00F9FF, 0x00F3FF, 0x00EDFF, 0x00E7FF, 0x00E1FF, 0x00DBFF, 0x00D5FF,
    0x00CFFF, 0x00C9FF, 0x00C3FF, 0x00BDFF, 0x00B7FF, 0x00B1FF, 0x00ABFF, 0x00A5FF,
    0x009FFF, 0x0099FF, 0x0093FF, 0x008DFF, 0x0087FF, 0x0081FF, 0x007CFF, 0x0076FF,
    0x0070FF, 0x006AFF, 0x0064FF, 0x005EFF, 0x0058FF, 0x0052FF, 0x004CFF, 0x0046FF,
    0x0040FF, 0x003AFF, 0x0034FF, 0x002EFF, 0x0028FF, 0x0022FF, 0x001CFF, 0x0016FF,
    0x0010FF, 0x000AFF, 0x0004FF, 0x0200FF, 0x0800FF, 0x0E00FF, 0x1400FF, 0x1A00FF,
    0x2000FF, 0x2600FF, 0x2C00FF, 0x3200FF, 0x3800FF, 0x3E00FF, 0x4400FF, 0x4A00FF,
    0x5000FF, 0x5600FF, 0x5C00FF, 0x6200FF, 0x6800FF, 0x6E00FF, 0x7400FF, 0x7A00FF,
    0x7F00FF, 0x8500FF, 0x8B00FF, 0x9100FF, 0x9700FF, 0x9D00FF, 0xA300FF, 0xA900FF,
    0xAF00FF, 0xB500FF, 0xBB00FF, 0xC100FF, 0xC700FF, 0xCD00FF, 0xD300FF, 0xD900FF,
    0xDF00FF, 0xE500FF, 0xEB00FF, 0xF100FF, 0xF700FF, 0xFD00FF, 0xFF00FB, 0xFF00F5,
    0xFF00EF, 0xFF00E9, 0xFF00E3, 0xFF00DD, 0xFF00D7, 0xFF00D1, 0xFF00CB, 0xFF00C5,
    0xFF00BF, 0xFF00B9, 0xFF00B3, 0xFF00AD, 0xFF00A7, 0xFF00A1, 0xFF009B, 0xFF0095,
    0xFF008F, 0xFF0089, 0xFF0083, 0xFF007E, 0xFF0078, 0xFF0072, 0xFF006C, 0xFF0066,
    0xFF0060, 0xFF005A, 0xFF0054, 0xFF004E, 0xFF0048, 0xFF0042, 0xFF003C, 0xFF0036,
    0xFF0030, 0xFF002A, 0xFF0024, 0xFF001E, 0xFF0018, 0xFF0012, 0xFF000C, 0xFF0006,
};

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

        fclose(file);
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
        if (ferror(stdin)) err(EX_IOERR, "(stdin)");
    }

    return EX_OK;
}

static char options[128];
static void formatOptions(void) {
    snprintf(
        options, sizeof(options),
        "gfxx -c %s -e%c -E%c -b %hhu%hhu%hhu%hhu -n %#zx %s%s-w %zu -z %zu",
        COLOR__STR[space],
        "lb"[byteOrder],
        "lb"[bitOrder],
        bits[PAD], bits[R], bits[G], bits[B],
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
            n |= (byteOrder == ENDIAN_BIG) ? data[i + b] : data[i + bytes - b - 1];
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

static struct {
    unsigned counter;
    char path[FILENAME_MAX];
    FILE *file;
} out;
static void outOpen(const char *ext) {
    snprintf(out.path, sizeof(out.path), "%s%04u.%s", prefix, ++out.counter, ext);
    out.file = fopen(out.path, "wx");
    if (out.file) {
        printf("%s\n", out.path);
    } else {
        warn("%s", out.path);
    }
}

static uint32_t crc;
static void pngWrite(const void *ptr, size_t size) {
    fwrite(ptr, size, 1, out.file);
    if (ferror(out.file)) err(EX_IOERR, "%s", out.path);
    crc = crc32(crc, ptr, size);
}
static void pngUint(uint32_t host) {
    uint32_t net = htonl(host);
    pngWrite(&net, 4);
}
static void pngChunk(const char *type, uint32_t size) {
    pngUint(size);
    crc = crc32(0, Z_NULL, 0);
    pngWrite(type, 4);
}

static void pngDump(uint32_t *src, size_t srcWidth, size_t srcHeight) {
    int error;

    size_t stride = 1 + 3 * srcWidth;
    uint8_t data[stride * srcHeight];
    for (size_t y = 0; y < srcHeight; ++y) {
        data[y * stride] = 0;
        for (size_t x = 0; x < srcWidth; ++x) {
            uint8_t *p = &data[y * stride + 1 + 3 * x];
            p[0] = src[y * srcWidth + x] >> 16;
            p[1] = src[y * srcWidth + x] >> 8;
            p[2] = src[y * srcWidth + x];
        }
    }

    uLong deflateSize = compressBound(sizeof(data));
    uint8_t deflate[deflateSize];
    error = compress(deflate, &deflateSize, data, sizeof(data));
    if (error != Z_OK) errx(EX_SOFTWARE, "compress: %d", error);

    outOpen("png");
    if (!out.file) return;

    const uint8_t SIGNATURE[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    const uint8_t HEADER[] = { 8, 2, 0, 0, 0 }; // 8-bit truecolor
    const char SOFTWARE[] = "Software";
    formatOptions();
    uint8_t sbit[3] = { MAX(bits[R], 1), MAX(bits[G], 1), MAX(bits[B], 1) };

    pngWrite(SIGNATURE, sizeof(SIGNATURE));

    pngChunk("IHDR", 4 + 4 + sizeof(HEADER));
    pngUint(srcWidth);
    pngUint(srcHeight);
    pngWrite(HEADER, sizeof(HEADER));
    pngUint(crc);

    pngChunk("tEXt", sizeof(SOFTWARE) + strlen(options));
    pngWrite(SOFTWARE, sizeof(SOFTWARE));
    pngWrite(options, strlen(options));
    pngUint(crc);

    pngChunk("sBIT", sizeof(sbit));
    pngWrite(sbit, sizeof(sbit));
    pngUint(crc);

    pngChunk("IDAT", deflateSize);
    pngWrite(deflate, deflateSize);
    pngUint(crc);

    pngChunk("IEND", 0);
    pngUint(crc);

    error = fclose(out.file);
    if (error) err(EX_IOERR, "%s", out.path);
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
    outOpen("dat");
    if (!out.file) return;

    fwrite(palette, 4, 256, out.file);
    if (ferror(out.file)) err(EX_IOERR, "%s", out.path);

    int error = fclose(out.file);
    if (error) err(EX_IOERR, "%s", out.path);
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
    bits[PAD] = PRESETS[preset][PAD];
    bits[R] = PRESETS[preset][R];
    bits[G] = PRESETS[preset][G];
    bits[B] = PRESETS[preset][B];
}

static void setBit(char in) {
    static uint8_t bit = 0;
    bits[bit++] = in - '0';
    bit &= 3;
}

bool input(char in) {
    size_t pixel = (BITS_TOTAL + 7) / 8;
    size_t row = width * BITS_TOTAL / 8;
    switch (in) {
        case 'q': return false;
        break; case 'x': dump = DUMP_ONE;
        break; case 'X': dump ^= DUMP_ALL;
        break; case 'o': formatOptions(); printf("%s\n", options);
        break; case '[': if (!space--) space = COLOR__COUNT - 1;
        break; case ']': if (++space == COLOR__COUNT) space = 0;
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
