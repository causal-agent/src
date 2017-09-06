#if 0
set -e
bin=$(dirname $0)
cc -Wall -Wextra -pedantic $@ -o $bin/pbd $0
ln -f $bin/pbd $bin/pbcopy
ln -f $bin/pbd $bin/pbpaste
exit
#endif

// TCP server which pipes between macOS pbcopy and pbpaste, and pbcopy and
// pbpaste implementations which connect to it.

#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

static void spawn(const char *cmd, int childFd, int parentFd) {
    pid_t pid = fork();
    if (pid < 0) err(EX_OSERR, "fork");

    if (pid) {
        int status;
        pid_t wait = waitpid(pid, &status, 0);
        if (wait < 0) err(EX_OSERR, "waitpid");

        if (status) {
            warnx("child %s status %d", cmd, status);
        }
    } else {
        int fd = dup2(parentFd, childFd);
        if (fd < 0) err(EX_OSERR, "dup2");

        int error = execlp(cmd, cmd, NULL);
        if (error) err(EX_OSERR, "execlp");
    }
}

static int pbd(void) {
    int error;

    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) err(EX_OSERR, "socket");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(7062),
        .sin_addr = { .s_addr = htonl(0x7f000001) },
    };

    error = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_OSERR, "bind");

    error = listen(server, 1);
    if (error) err(EX_OSERR, "listen");

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) err(EX_OSERR, "accept");

        spawn("pbpaste", STDOUT_FILENO, client);

        uint8_t p;
        ssize_t peek = recv(client, &p, 1, MSG_PEEK);
        if (peek < 0) err(EX_IOERR, "recv");

        if (peek) {
            spawn("pbcopy", STDIN_FILENO, client);
        }

        error = close(client);
        if (error) err(EX_IOERR, "close");
    }
}

static int pbdClient(void) {
    int client = socket(PF_INET, SOCK_STREAM, 0);
    if (client < 0) err(EX_OSERR, "socket");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(7062),
        .sin_addr = { .s_addr = htonl(0x7f000001) },
    };

    int error = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_OSERR, "connect");

    return client;
}

static void copy(int fdIn, int fdOut) {
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
}

static int pbcopy(void) {
    int client = pbdClient();
    copy(STDIN_FILENO, client);
    return EX_OK;
}

static int pbpaste(void) {
    int client = pbdClient();
    int error = shutdown(client, SHUT_WR);
    if (error) err(EX_OSERR, "shutdown");
    copy(client, STDOUT_FILENO);
    return EX_OK;
}

int main(int argc __attribute((unused)), char *argv[]) {
    if (!argv[0][0] || !argv[0][1]) return EX_USAGE;
    switch (argv[0][2]) {
        case 'd': return pbd();
        case 'c': return pbcopy();
        case 'p': return pbpaste();
    }
    return EX_USAGE;
}
