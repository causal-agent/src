#if 0
exec clang -Weverything $@ -o $(dirname $0)/pbd $0
#endif

#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sysexits.h>
#include <unistd.h>

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

        pid_t pid_paste = fork();
        if (pid_paste < 0) err(EX_OSERR, "fork");

        if (pid_paste) {
            if (waitpid(pid_paste, NULL, 0) < 0)
                warn("waitpid");
            // TODO: Check child status.
        } else {
            if (dup2(client, STDOUT_FILENO) < 0)
                err(EX_OSERR, "dup2");
            if (execlp("pbpaste", "pbpaste") < 0)
                err(EX_OSERR, "execlp");
        }

        uint8_t x;
        ssize_t peek = recv(client, &x, 1, MSG_PEEK);
        if (peek < 0) err(EX_IOERR, "recv");

        if (peek) {
            pid_t pid_copy = fork();
            if (pid_copy < 0) err(EX_OSERR, "fork");

            if (pid_copy) {
                if (waitpid(pid_copy, NULL, 0) < 0)
                    warn("waitpid");
                // TODO: Check child status.
            } else {
                if (dup2(client, STDIN_FILENO) < 0)
                    err(EX_OSERR, "dup2");
                if (execlp("pbcopy", "pbcopy") < 0)
                    err(EX_OSERR, "execlp");
            }
        }

        if (close(client) < 0) warn("close");
    }
}
