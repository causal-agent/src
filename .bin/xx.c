#if 0
exec cc -Weverything -Wno-vla -o ~/.bin/xx $0
#endif

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool zero(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        if (buf[i]) return false;
    return true;
}

enum {
    ASCII = 1,
    OFFSETS = 2,
    SKIP = 4,
};

int main(int argc, char **argv)
{
    size_t cols = 16;
    size_t group = 8;
    uint8_t flags = ASCII | OFFSETS;
    char *path = NULL;

    while (getopt(argc, argv, "ac:g:os") > 0)
        if (optopt == 'a') {
            flags ^= ASCII;
        } else if (optopt == 'c') {
            cols = (size_t) strtol(optarg, NULL, 10);
            if (!cols) return EXIT_FAILURE;
        } else if (optopt == 'g') {
            group = (size_t) strtol(optarg, NULL, 10);
        } else if (optopt == 'o') {
            flags ^= OFFSETS;
        } else if (optopt == 's') {
            flags ^= SKIP;
        } else return EXIT_FAILURE;
    if (argc > optind)
        path = argv[optind];

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) {
        perror(path);
        return EXIT_FAILURE;
    }

    uint8_t buf[cols];
    size_t offset = 0, n = 0, i;
    bool skip = false;
    for (;;) {
        offset += n;
        n = fread(buf, 1, sizeof(buf), file);

        if ((flags & SKIP) && n == sizeof(buf)) {
            if (zero(buf, n)) {
                if (!skip) printf("*\n");
                skip = true;
                continue;
            } else skip = false;
        }

        if (flags & OFFSETS)
            printf("%08zx:  ", offset);
        for (i = 0; i < n; ++i) {
            if (group && i && !(i % group)) printf(" ");
            printf("%02x ", buf[i]);
        }

        if (flags & ASCII) {
            // TODO: Fix alignment with group.
            for (i = n; i < cols; ++i)
                printf("   ");
            printf(" ");
            for (i = 0; i < n; ++i)
                printf("%c", isprint(buf[i]) ? buf[i] : '.');
        }

        printf("\n");
        if (n < sizeof(buf)) break;
    }
    if (ferror(file)) {
        perror(path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
