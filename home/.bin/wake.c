#if 0
exec cc -Wall -Wextra -Wpedantic $@ -o $(dirname $0)/wake $0
#endif

#include <err.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sysexits.h>

#define MAC 0x04, 0x7D, 0x7B, 0xD5, 0x6A, 0x53

const uint8_t payload[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    MAC, MAC, MAC, MAC, MAC, MAC, MAC, MAC,
    MAC, MAC, MAC, MAC, MAC, MAC, MAC, MAC,
};

int main() {
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) err(EX_OSERR, "socket");

    int on = 1;
    int error = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    if (error) err(EX_OSERR, "setsockopt");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = 9,
        .sin_addr.s_addr = INADDR_BROADCAST,
    };

    ssize_t len = sendto(
        sock, payload, sizeof(payload), 0,
        (struct sockaddr *)&addr, sizeof(addr)
    );
    if (len < 0) err(EX_IOERR, "sendto");

    return EX_OK;
}
