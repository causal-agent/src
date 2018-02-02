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
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

static int init(int argc, char *argv[]);
static void draw(uint32_t *buf, size_t xres, size_t yres);
static void input(char in);

static uint32_t *buf;
static struct fb_var_screeninfo info;

static uint32_t saveBg;
static void restoreBg(void) {
    for (size_t i = 0; i < info.xres * info.yres; ++i) {
        buf[i] = saveBg;
    }
}

static struct termios saveTerm;
static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
    int error;

    error = init(argc, argv);
    if (error) return error;

    const char *path = getenv("FRAMEBUFFER");
    if (!path) path = "/dev/fb0";

    int fb = open(path, O_RDWR);
    if (fb < 0) err(EX_OSFILE, "%s", path);

    error = ioctl(fb, FBIOGET_VSCREENINFO, &info);
    if (error) err(EX_IOERR, "%s", path);

    size_t size = 4 * info.xres * info.yres;
    buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (buf == MAP_FAILED) err(EX_IOERR, "%s", path);

    saveBg = buf[0];
    atexit(restoreBg);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios term = saveTerm;
    term.c_lflag &= ~(ICANON | ECHO);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &term);
    if (error) err(EX_IOERR, "tcsetattr");

    for (;;) {
        draw(buf, info.xres, info.yres);

        char in;
        ssize_t len = read(STDERR_FILENO, &in, 1);
        if (len < 0) err(EX_IOERR, "read");
        if (!len) return EX_DATAERR;

        input(in);
    }
}

// ABSTRACTION LINE

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static enum {
    COLOR_GRAYSCALE,
    COLOR_PALETTE,
    COLOR_RGB,
    COLOR__MAX,
} space;
static uint32_t palette[256];
static uint8_t bits = 1;
static bool endian;

static bool reverse;
static size_t offset;

static size_t width = 16;
static bool mirror;
static size_t scale = 1;

static size_t size = 1024 * 1024; // max from pipe.
static uint8_t *data;

static uint8_t get(size_t i) {
    if (reverse) {
        return data[size - i - 1];
    } else {
        return data[i];
    }
}

static /**/ int init(int argc, char *argv[]) {
    const char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "c:b:ern:mw:z:"))) {
        switch (opt) {
            case 'c': switch (optarg[0]) {
                case 'g': space = COLOR_GRAYSCALE; break;
                case 'p': space = COLOR_PALETTE; break;
                case 'r': space = COLOR_RGB; break;
                default: return EX_USAGE;
            } break;
            case 'b': bits     = strtoul(optarg, NULL, 0); break;
            case 'e': endian  ^= true; break;
            case 'r': reverse ^= true; break;
            case 'n': offset   = strtoul(optarg, NULL, 0); break;
            case 'w': width    = strtoul(optarg, NULL, 0); break;
            case 'm': mirror  ^= true; break;
            case 'z': scale    = strtoul(optarg, NULL, 0); break;
            default: return EX_USAGE;
        }
    }
    if (argc > optind) path = argv[optind];
    if (!bits || !width || !scale) return EX_USAGE;

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

    return EX_OK;
}

static void printOpts(void) {
    printf(
        "gfxx -c %c -b %hhu %s%s-n %#zx -w %zu %s-z %zu\n",
        "gpr"[space],
        bits,
        endian ? "-e " : "",
        reverse ? "-r " : "",
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

static void next(struct Pos *pos) {
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

static void draw1(struct Pos *pos) {
    for (size_t i = offset; i < size; ++i) {
        for (int s = 0; s < 8; ++s) {
            uint8_t b = get(i) >> (endian ? s : 7 - s) & 1;
            if (space == COLOR_PALETTE) {
                put(pos, palette[b]);
            } else {
                space = COLOR_GRAYSCALE;
                put(pos, b ? 0xFFFFFF : 0x000000);
            }
            next(pos);
        }
    }
}

static void draw4(struct Pos *pos) {
    for (size_t i = offset; i < size; ++i) {
        uint32_t a = (endian) ? get(i) >>   4 : get(i) & 0x0F;
        uint32_t b = (endian) ? get(i) & 0x0F : get(i) >>   4;
        if (space == COLOR_PALETTE) {
            put(pos, palette[a]);
            next(pos);
            put(pos, palette[b]);
            next(pos);
        } else {
            space = COLOR_GRAYSCALE;
            a = 256 * a / 8;
            b = 256 * b / 8;
            put(pos, a << 16 | a << 8 | a);
            next(pos);
            put(pos, b << 16 | b << 8 | b);
            next(pos);
        }
    }
}

static void draw8(struct Pos *pos) {
    for (size_t i = offset; i < size; ++i) {
        uint32_t p = 0;
        if (space == COLOR_GRAYSCALE) {
            p = get(i) | get(i) << 8 | get(i) << 16;
        } else if (space == COLOR_PALETTE) {
            p = palette[get(i)];
        } else if (space == COLOR_RGB) {
            // FIXME: This might be totally wrong.
            // RRRGGGBB
            uint32_t r = (endian) ? get(i) >> 5 & 0x07 : get(i) >> 0 & 0x07;
            uint32_t g = (endian) ? get(i) >> 2 & 0x07 : get(i) >> 3 & 0x07;
            uint32_t b = (endian) ? get(i) >> 0 & 0x03 : get(i) >> 6 & 0x03;
            p = (256 * r / 8) << 16 | (256 * g / 8) << 8 | (256 * b / 4);
        }
        put(pos, p);
        next(pos);
    }
}

static void draw16(struct Pos *pos) {
    for (size_t i = offset; i + 1 < size; i += 2) {
        // FIXME: Endian means this, or BGR?
        uint16_t x = (endian)
            ? (uint16_t)get(i+0) << 8 | (uint16_t)get(i+1)
            : (uint16_t)get(i+1) << 8 | (uint16_t)get(i+0);
        // FIXME: This might be totally wrong.
        // RRRRRGGGGGGBBBBB
        space = COLOR_RGB;
        uint32_t r = x >> 11 & 0x1F;
        uint32_t g = x >>  5 & 0x3F;
        uint32_t b = x >>  0 & 0x1F;
        uint32_t p = (256 * r / 32) << 16 | (256 * g / 64) << 8 | (256 * b / 32);
        put(pos, p);
        next(pos);
    }
}

static void draw24(struct Pos *pos) {
    for (size_t i = offset; i + 2 < size; i += 3) {
        space = COLOR_RGB;
        uint32_t p = (endian)
            ? get(i+0) << 16 | get(i+1) << 8 | get(i+2) << 0
            : get(i+2) << 16 | get(i+1) << 8 | get(i+0) << 0;
        put(pos, p);
        next(pos);
    }
}

static void draw32(struct Pos *pos) {
    for (size_t i = offset; i + 3 < size; i += 4) {
        space = COLOR_RGB;
        uint32_t p = (endian)
            ? get(i+0) << 24 | get(i+1) << 16 | get(i+2) << 8 | get(i+3) << 0
            : get(i+3) << 24 | get(i+2) << 16 | get(i+1) << 8 | get(i+0) << 0;
        put(pos, p);
        next(pos);
    }
}

static /**/ void draw(uint32_t *buf, size_t xres, size_t yres) {
    memset(buf, 0, 4 * xres * yres);
    struct Pos pos = {
        .buf = buf,
        .xres = xres,
        .yres = yres,
        .x = (mirror) ? width - 1 : 0
    };
    switch (bits) {
        case 1:  draw1(&pos);  break;
        case 4:  draw4(&pos);  break;
        case 8:  draw8(&pos);  break;
        case 16: draw16(&pos); break;
        case 24: draw24(&pos); break;
        case 32: draw32(&pos); break;
        default: break;
    }
}

static /**/ void input(char in) {
    switch (in) {
        case 'q': printOpts(); exit(EX_OK);
        break; case '+': scale++;
        break; case '-': if (scale > 1) scale--;
        break; case '.': width++;
        break; case ',': if (width > 1) width--;
        break; case '>': width *= 2;
        break; case '<': if (width / 2 >= 1) width /= 2;
        break; case 'h': if (offset) offset--;
        break; case 'j': offset += width;
        break; case 'k': if (offset >= width) offset -= width;
        break; case 'l': offset++;
        break; case 'e': endian ^= true;
        break; case 'r': reverse ^= true;
        break; case 'm': mirror ^= true;
        break; case 'c': space++; if (space == COLOR__MAX) space = 0;
        break; case 'b':
            switch (bits) {
                case 1:  bits =  4; break;
                case 4:  bits =  8; break;
                case 32: bits =  1; break;
                default: bits += 8;
            }
        break; case 'B':
            switch (bits) {
                case 8:  bits =  4; break;
                case 4:  bits =  1; break;
                case 1:  bits = 32; break;
                default: bits -= 8;
            }
    }
}
