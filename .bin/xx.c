#if 0
exec cc -Weverything -Wno-vla -o ~/.bin/xx $0
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    size_t cols = 16;
    size_t group = 8;
    char *path = NULL;

    while (getopt(argc, argv, "c:g:") > 0)
        if (optopt == 'c') {
            cols = (size_t) strtol(optarg, NULL, 10);
            if (!cols) return EXIT_FAILURE;
        } else if (optopt == 'g') {
            group = (size_t) strtol(optarg, NULL, 10);
        } else return EXIT_FAILURE;
    if (argc > optind)
        path = argv[optind];

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) {
        perror(path);
        return EXIT_FAILURE;
    }

    uint8_t buf[cols];
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), file);

        for (size_t i = 0; i < n; ++i) {
            if (group && i && !(i % group)) printf(" ");
            printf("%02x ", buf[i]);
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
