#if 0
cc -Wall -Wextra -pedantic $@ -o $(dirname $0)/pbcopy $0 && \
exec cc -Wall -Wextra -pedantic -DPBPASTE $@ -o $(dirname $0)/pbpaste $0
#endif

#include <arpa/inet.h>
#include <err.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sysexits.h>
#include <unistd.h>

int main() {
    int error;

    int client = socket(PF_INET, SOCK_STREAM, 0);
    if (client < 0) err(EX_OSERR, "socket");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(7062),
        .sin_addr = { .s_addr = htonl(0x7f000001) },
    };

    error = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_OSERR, "connect");

#ifdef PBPASTE
    int fdIn = client;
    int fdOut = STDOUT_FILENO;
    error = shutdown(client, SHUT_WR);
    if (error) err(EX_OSERR, "shutdown");
#else
    int fdIn = STDIN_FILENO;
    int fdOut = client;
#endif

    char readBuf[4096];
    ssize_t readLen;
    while (0 < (readLen = read(fdIn, readBuf, sizeof(readBuf)))) {
        char *writeBuf = readBuf;
        ssize_t writeLen;
        while (0 < (writeLen = write(fdOut, writeBuf, readLen))) {
            writeBuf += writeLen;
            readLen -= writeLen;
        }
        if (writeLen < 0) err(EX_IOERR, "write");
    }
    if (readLen < 0) err(EX_IOERR, "read");

    return 0;
}
