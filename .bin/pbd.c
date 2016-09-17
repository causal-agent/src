#if 0
exec clang -Weverything -Wno-missing-prototypes $@ -o $(dirname $0)/pbd $0
#endif

#include <err.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sysexits.h>
#include <unistd.h>

void spawn(const char *cmd, int child_fd, int parent_fd) {
    pid_t pid = fork();
    if (pid < 0) err(EX_OSERR, "fork");

    if (pid) {
        if (waitpid(pid, NULL, 0) < 0)
            err(EX_OSERR, "waitpid");
        // TODO: Check child status.
    } else {
        if (dup2(parent_fd, child_fd) < 0)
            err(EX_OSERR, "dup2");
        if (execlp(cmd, cmd) < 0)
            err(EX_OSERR, "execlp");
    }
}

int main() {
    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) err(EX_OSERR, "socket");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(7062),
        .sin_addr = { .s_addr = htonl(0x7f000001) },
    };

    if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        err(EX_OSERR, "bind");

    if (listen(server, 1) < 0)
        err(EX_OSERR, "listen");

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) err(EX_OSERR, "accept");

        spawn("pbpaste", STDOUT_FILENO, client);

        uint8_t x;
        ssize_t peek = recv(client, &x, 1, MSG_PEEK);
        if (peek < 0) err(EX_IOERR, "recv");

        if (peek) spawn("pbcopy", STDIN_FILENO, client);

        if (close(client) < 0) warn("close");
    }
}
