#if 0
exec cc -Weverything -Wno-vla -o ~/.bin/xx $0
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum {
    OFFSETS = 1,
};

int main(int argc, char **argv)
{
    size_t cols = 16;
    size_t group = 8;
    uint8_t flags = OFFSETS;
    char *path = NULL;

    while (getopt(argc, argv, "c:g:o") > 0)
        if (optopt == 'c') {
            cols = (size_t) strtol(optarg, NULL, 10);
            if (!cols) return EXIT_FAILURE;
        } else if (optopt == 'g') {
            group = (size_t) strtol(optarg, NULL, 10);
        } else if (optopt == 'o') {
            flags ^= OFFSETS;
        } else return EXIT_FAILURE;
    if (argc > optind)
        path = argv[optind];

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) {
        perror(path);
        return EXIT_FAILURE;
    }

    uint8_t buf[cols];
    size_t offset = 0, n, i;
    for (;;) {
        n = fread(buf, 1, sizeof(buf), file);

        if (flags & OFFSETS)
            printf("%08zx:  ", offset);
        for (i = 0; i < n; ++i) {
            if (group && i && !(i % group)) printf(" ");
            printf("%02x ", buf[i]);
        }

        printf("\n");
        offset += n;
        if (n < sizeof(buf)) break;
    }
    if (ferror(file)) {
        perror(path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
