/* Copyright (c) 2017, June McEnroe <programble@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#define UNUSED __attribute__((unused))

static void spawn(const char *cmd, int dest, int src) {
    pid_t pid = fork();
    if (pid < 0) err(EX_OSERR, "fork");

    if (pid) {
        int status;
        pid_t dead = waitpid(pid, &status, 0);
        if (dead < 0) err(EX_OSERR, "waitpid(%d)", pid);
        if (status) warnx("%s: status %d", cmd, status);

    } else {
        int fd = dup2(src, dest);
        if (fd < 0) err(EX_OSERR, "dup2");

        execlp(cmd, cmd, NULL);
        err(EX_UNAVAILABLE, "%s", cmd);
    }
}

static int pbd(void) {
    int error;

    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) err(EX_OSERR, "socket");

    error = fcntl(server, F_SETFD, FD_CLOEXEC);
    if (error) err(EX_IOERR, "fcntl");

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(7062),
        .sin_addr = { .s_addr = htonl(0x7f000001) },
    };
    error = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_UNAVAILABLE, "bind");

    error = listen(server, 0);
    if (error) err(EX_UNAVAILABLE, "listen");

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) err(EX_IOERR, "accept");

        error = fcntl(client, F_SETFD, FD_CLOEXEC);
        if (error) err(EX_IOERR, "fcntl");

        spawn("pbpaste", STDOUT_FILENO, client);

        char p;
        ssize_t peek = recv(client, &p, 1, MSG_PEEK);
        if (peek < 0) err(EX_IOERR, "recv");

        if (peek) spawn("pbcopy", STDIN_FILENO, client);

        close(client);
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
    if (error) err(EX_UNAVAILABLE, "connect");

    return client;
}

static void copy(int out, int in) {
    char buf[4096];
    ssize_t readSize;
    while (0 < (readSize = read(in, buf, sizeof(buf)))) {
        ssize_t writeSize = write(out, buf, readSize);
        if (writeSize < 0) err(EX_IOERR, "write(%d)", out);
        if (writeSize < readSize) errx(EX_IOERR, "short write(%d)", out);
    }
    if (readSize < 0) err(EX_IOERR, "read(%d)", in);
}

static int pbcopy(void) {
    int client = pbdClient();
    copy(client, STDIN_FILENO);
    return EX_OK;
}

static int pbpaste(void) {
    int client = pbdClient();
    shutdown(client, SHUT_WR);
    copy(STDOUT_FILENO, client);
    return EX_OK;
}

int main(int argc UNUSED, char *argv[]) {
    if (!argv[0][0] || !argv[0][1]) return EX_USAGE;
    switch (argv[0][2]) {
        case 'd': return pbd();
        case 'c': return pbcopy();
        case 'p': return pbpaste();
        default:  return EX_USAGE;
    }
}
