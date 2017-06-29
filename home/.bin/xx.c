#if 0
exec cc -Wall -Wextra -pedantic $@ -o $(dirname $0)/xx $0
#endif

// Hexdump.

#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

static bool allZero(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i)
        if (buf[i]) return false;
    return true;
}

enum {
    FLAG_ASCII  = 1 << 0,
    FLAG_COLOR  = 1 << 1,
    FLAG_OFFSET = 1 << 2,
    FLAG_SKIP   = 1 << 3,
    FLAG_UNDUMP = 1 << 4,
};

static void color(size_t i) {
    printf("\x1b[%c%cm", (i % 8) ? '3' : '9', (char)(i % 8 + '0'));
}

static void dump(size_t cols, size_t group, uint8_t flags, FILE *file) {
    uint8_t buf[cols];
    size_t offset = 0, len = 0;
    for (;;) {
        offset += len;
        len = fread(buf, 1, sizeof(buf), file);
        if (!len) break;

        if (flags & FLAG_COLOR) printf("\x1b[39m");

        if ((flags & FLAG_SKIP) && len == sizeof(buf)) {
            static bool skip;
            if (allZero(buf, len)) {
                if (!skip) printf("*\n");
                skip = true;
                continue;
            }
            skip = false;
        }

        if (flags & FLAG_OFFSET) {
            printf("%08zx:  ", offset);
        }

        for (size_t i = 0; i < len; ++i) {
            if (group && i && !(i % group)) printf(" ");
            if (flags & FLAG_COLOR) color(i);
            printf("%02x ", buf[i]);
        }

        if (flags & FLAG_ASCII) {
            for (size_t i = len; i < cols; ++i) {
                printf((group && !(i % group)) ? "    " : "   ");
            }
            printf(" ");
            for (size_t i = 0; i < len; ++i) {
                if (flags & FLAG_COLOR) color(i);
                printf("%c", isprint(buf[i]) ? buf[i] : '.');
            }
        }

        printf("\n");
        if (len < sizeof(buf)) break;
    }
}

static void undump(FILE *file) {
    uint8_t byte;
    int match;
    while (1 == (match = fscanf(file, " %hhx", &byte))) {
        printf("%c", byte);
    }
    if (match == 0) errx(EX_DATAERR, "invalid input");
}

int main(int argc, char *argv[]) {
    size_t cols = 16;
    size_t group = 8;
    uint8_t flags = FLAG_ASCII | FLAG_OFFSET;
    char *path = NULL;

    int opt;
    while (0 < (opt = getopt(argc, argv, "aCc:fg:hku"))) {
        switch (opt) {
            case 'a': flags ^= FLAG_ASCII; break;
            case 'C': flags ^= FLAG_COLOR; break;
            case 'f': flags ^= FLAG_OFFSET; break;
            case 'k': flags ^= FLAG_SKIP; break;
            case 'u': flags ^= FLAG_UNDUMP; break;
            case 'c': cols = strtoul(optarg, NULL, 10); break;
            case 'g': group = strtoul(optarg, NULL, 10); break;
            default:
                fprintf(stderr, "usage: xx [-aCfku] [-c cols] [-g group] [file]\n");
                return (opt == 'h') ? EX_OK : EX_USAGE;
        }
    }
    if (!cols) return EX_USAGE;
    if (argc > optind) {
        path = argv[optind];
    }

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) err(EX_NOINPUT, "%s", path);

    if (flags & FLAG_UNDUMP) {
        undump(file);
    } else {
        dump(cols, group, flags, file);
    }

    if (ferror(file)) err(EX_IOERR, "%s", path);
    return EX_OK;
}
