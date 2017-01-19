#if 0
exec cc -Wall -Wextra -pedantic $@ -o $(dirname $0)/clock $0
#endif

#include <time.h>
#include <stdio.h>
#include <sysexits.h>
#include <err.h>

int main() {
    time_t timestamp = time(NULL);
    if (timestamp < 0) err(EX_OSERR, "time");

    struct tm *clock = localtime(&timestamp);
    if (!clock) err(EX_OSERR, "localtime");

    switch ((clock->tm_min + 5) / 10) {
        case 0: printf("...%02d...\n", clock->tm_hour); break;
        case 1: printf("..%02d....\n", clock->tm_hour); break;
        case 2: printf(".%02d.....\n", clock->tm_hour); break;
        case 3: printf("%d......%d\n", clock->tm_hour % 10, (clock->tm_hour + 1) / 10); break;
        case 4: printf(".....%02d.\n", clock->tm_hour + 1); break;
        case 5: printf("....%02d..\n", clock->tm_hour + 1); break;
        case 6: printf("...%02d...\n", clock->tm_hour + 1); break;
    }

    return 0;
}
