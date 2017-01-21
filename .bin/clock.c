#if 0
exec cc -Wall -Wextra -pedantic $@ -o $(dirname $0)/clock $0
#endif

// Fuzzy clock for display in tmux.

#include <time.h>
#include <stdio.h>
#include <sysexits.h>
#include <err.h>

int main() {
    time_t timestamp = time(NULL);
    if (timestamp < 0) err(EX_OSERR, "time");

    struct tm *clock = localtime(&timestamp);
    if (!clock) err(EX_OSERR, "localtime");

    int hour = clock->tm_hour;
    int next = (hour + 1) % 24;

    switch ((clock->tm_min + 5) / 10) {
        case 0: printf("..%02d..\n", hour); break;
        case 1: printf(".%02d...\n", hour); break;
        case 2: printf("%02d....\n", hour); break;
        case 3: printf("%d....%d\n", hour % 10, next / 10); break;
        case 4: printf("....%02d\n", next); break;
        case 5: printf("...%02d.\n", next); break;
        case 6: printf("..%02d..\n", next); break;
    }

    return 0;
}
