#if 0
exec cc -Weverything -o ~/.bin/xx $0
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *path = NULL;

    if (argc > 1)
        path = argv[1];

    FILE *file = path ? fopen(path, "r") : stdin;
    if (!file) {
        perror(path);
        return EXIT_FAILURE;
    }

    uint8_t buf[16];
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), file);
        for (size_t i = 0; i < n; ++i)
            printf("%02x ", buf[i]);
        printf("\n");
        if (n < sizeof(buf)) break;
    }
    if (ferror(file)) {
        perror(path);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
