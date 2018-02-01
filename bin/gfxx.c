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

struct Buffer {
    uint32_t *data;
    size_t xres;
    size_t yres;
};

static int init(int argc, char *argv[]);
static void draw(struct Buffer buf);
static void input(char in);

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

    struct fb_var_screeninfo info;
    error = ioctl(fb, FBIOGET_VSCREENINFO, &info);
    if (error) err(EX_IOERR, "%s", path);

    size_t size = 4 * info.xres * info.yres;
    struct Buffer buf = {
        .data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0),
        .xres = info.xres,
        .yres = info.yres,
    };
    if (buf.data == MAP_FAILED) err(EX_IOERR, "%s", path);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios term = saveTerm;
    term.c_lflag &= ~(ICANON | ECHO);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &term);
    if (error) err(EX_IOERR, "tcsetattr");

    for (;;) {
        draw(buf);

        char in;
        ssize_t len = read(STDERR_FILENO, &in, 1);
        if (len < 0) err(EX_IOERR, "read");
        if (!len) return EX_DATAERR;

        input(in);
    }
}

// ABSTRACTION LINE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uint8_t bits = 1;
static bool endian;
static bool reverse;
static size_t width = 16;
static size_t offset;
static size_t scale = 1;

static size_t size = 1024 * 1024;
static uint8_t *data;

static uint8_t get(size_t i) {
    if (reverse) {
        return data[size - i - 1];
    } else {
        return data[i];
    }
}

static int init(int argc, char *argv[]) {
    const char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "b:en:rw:z:"))) {
        switch (opt) {
            case 'b': bits     = strtoul(optarg, NULL, 0); break;
            case 'e': endian  ^= true; break;
            case 'n': offset   = strtoul(optarg, NULL, 0); break;
            case 'r': reverse ^= true; break;
            case 'w': width    = strtoul(optarg, NULL, 0); break;
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

static void dump(void) {
    printf(
        "gfxx %s%s-b %hhu -n %zu -w %zu -z %zu\n",
        reverse ? "-r " : "",
        endian ? "-e " : "",
        bits,
        offset,
        width,
        scale
    );
}

struct Cursor {
    size_t left;
    size_t x;
    size_t y;
};

static struct Cursor step(struct Buffer buf, struct Cursor cur) {
    cur.x++;
    if (cur.x - cur.left == width) {
        cur.y++;
        cur.x = cur.left;
    }
    if (cur.y == buf.yres / scale) {
        cur.left += width;
        cur.x = cur.left;
        cur.y = 0;
    }
    return cur;
}

static void put(struct Buffer buf, struct Cursor cur, uint32_t p) {
    size_t scaledX = cur.x * scale;
    size_t scaledY = cur.y * scale;
    for (size_t fillY = scaledY; fillY < scaledY + scale; ++fillY) {
        if (fillY >= buf.yres) break;
        for (size_t fillX = scaledX; fillX < scaledX + scale; ++fillX) {
            if (fillX >= buf.xres) break;
            buf.data[fillY * buf.xres + fillX] = p;
        }
    }
}

// TODO: Palette.
static void draw1(struct Buffer buf) {
    struct Cursor cur = {0};
    for (size_t i = offset; i < size; ++i) {
        for (int s = 0; s < 8; ++s) {
            uint8_t b = get(i) >> (endian ? s : 7 - s) & 1;
            uint32_t p = b ? 0xFFFFFF : 0x000000;
            put(buf, cur, p);
            cur = step(buf, cur);
        }
    }
}

// TODO: Palette.
static void draw8(struct Buffer buf) {
    struct Cursor cur = {0};
    for (size_t i = offset; i < size; ++i) {
        uint32_t p = get(i) | get(i) << 8 | get(i) << 16;
        put(buf, cur, p);
        cur = step(buf, cur);
    }
}

static void draw24(struct Buffer buf) {
    struct Cursor cur = {0};
    for (size_t i = offset; i + 2 < size; i += 3) {
        uint32_t p = (endian)
            ? get(i) << 16 | get(i+1) <<  8 | get(i+2) <<  0
            : get(i) <<  0 | get(i+1) <<  8 | get(i+2) << 16;
        put(buf, cur, p);
        cur = step(buf, cur);
    }
}

static void draw32(struct Buffer buf) {
    struct Cursor cur = {0};
    for (size_t i = offset; i + 3 < size; i += 4) {
        uint32_t p = (endian)
            ? get(i) << 24 | get(i+1) << 16 | get(i+2) <<  8 | get(i+3) <<  0
            : get(i) <<  0 | get(i+1) <<  8 | get(i+2) << 16 | get(i+3) << 24;
        put(buf, cur, p);
        cur = step(buf, cur);
    }
}

static void draw(struct Buffer buf) {
    memset(buf.data, 0, 4 * buf.xres * buf.yres);
    switch (bits) {
        case 1:  draw1(buf);  break;
        case 8:  draw8(buf);  break;
        case 24: draw24(buf); break;
        case 32: draw32(buf); break;
        default: break;
    }
}

static void input(char in) {
    switch (in) {
        case 'q': dump(); exit(EX_OK);
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
        break; case 'b':
            switch (bits) {
                case 1:  bits =  8; break;
                case 32: bits =  1; break;
                default: bits += 8;
            }
        break; case 'B':
            switch (bits) {
                case 1:  bits = 32; break;
                case 8:  bits =  1; break;
                default: bits -= 8;
            }
    }
}
