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

static const char *CLASS = "/sys/class/backlight";

int main(int argc, char *argv[]) {
    int error;

    if (argc < 2) errx(EX_USAGE, "usage: bri N | +... | -...");

    error = chdir(CLASS);
    if (error) err(EX_IOERR, "%s", CLASS);

    DIR *dir = opendir(".");
    if (!dir) err(EX_IOERR, "%s", CLASS);

    struct dirent *entry;
    while (NULL != (errno = 0, entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;

        error = chdir(entry->d_name);
        if (error) err(EX_IOERR, "%s", entry->d_name);
        break;
    }
    if (!entry) {
        if (errno) err(EX_IOERR, "%s", CLASS);
        errx(EX_CONFIG, "empty %s", CLASS);
    }

    char *value = argv[1];

    if (value[0] == '+' || value[0] == '-') {
        FILE *actual = fopen("actual_brightness", "r");
        if (!actual) err(EX_IOERR, "actual_brightness");

        unsigned int brightness;
        int match = fscanf(actual, "%u", &brightness);
        if (match == EOF) err(EX_IOERR, "actual_brightness");
        if (match < 1) err(EX_DATAERR, "actual_brightness");

        size_t count = strnlen(value, 15);
        if (value[0] == '+') {
            brightness += 16 * count;
        } else {
            brightness -= 16 * count;
        }

        char buf[15];
        snprintf(buf, sizeof(buf), "%u", brightness);

        value = buf;
    }

    error = setuid(0);
    if (error) err(EX_NOPERM, "setuid(0)");

    FILE *brightness = fopen("brightness", "w");
    if (!brightness) err(EX_IOERR, "brightness");

    int count = fprintf(brightness, "%s", value);
    if (count < 0) err(EX_IOERR, "brightness");

    return EX_OK;
}
