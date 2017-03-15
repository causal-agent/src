#if 0
cc -Wall -Wextra -pedantic $@ -o $(dirname $0)/bri $0 && \
sudo chown root:root $(dirname $0)/bri && \
sudo chmod u+s $(dirname $0)/bri
exit
#endif

// Backlight brightness control.

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int error;

    if (argc < 2) errx(EX_USAGE, "usage: bri N | +... | -...");

    error = chdir("/sys/class/backlight");
    if (error) err(EX_IOERR, "/sys/class/backlight");

    DIR *dir = opendir(".");
    if (!dir) err(EX_IOERR, "opendir");

    struct dirent *entry;
    while (NULL != (errno = 0, entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        error = chdir(entry->d_name);
        if (error) err(EX_IOERR, entry->d_name);
        break;
    }
    if (!entry) {
        if (errno) err(EX_IOERR, "readdir");
        errx(EX_CONFIG, "empty /sys/class/backlight");
    }

    char *bright = argv[1];

    if (bright[0] == '+' || bright[0] == '-') {
        FILE *actual = fopen("actual_brightness", "r");
        if (!actual) err(EX_IOERR, "actual_brightness");

        unsigned int current;
        int match = fscanf(actual, "%u", &current);
        if (match == EOF) err(EX_IOERR, "fscanf");
        if (match < 1) err(EX_DATAERR, "fscanf");

        size_t count = strnlen(bright, 15);
        if (bright[0] == '+') {
            current += 16 * count;
        } else {
            current -= 16 * count;
        }

        char buf[15];
        snprintf(buf, sizeof(buf), "%u", current);

        bright = buf;
    }

    error = setuid(0);
    if (error) err(EX_NOPERM, "setuid");

    FILE *brightness = fopen("brightness", "w");
    if (!brightness) err(EX_IOERR, "brightness");

    int count = fprintf(brightness, "%s", bright);
    if (count < 0) err(EX_IOERR, "fprintf");

    return EX_OK;
}
