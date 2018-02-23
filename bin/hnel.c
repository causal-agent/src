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

#include <err.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#if defined __FreeBSD__
#include <libutil.h>
#elif defined __linux__
#include <pty.h>
#else
#include <util.h>
#endif

static struct termios saveTerm;
static void restoreTerm(void) {
    tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
    int error;

    if (argc < 4) return EX_USAGE;

    char table[256] = {0};
    if (strlen(argv[1]) != strlen(argv[2])) return EX_USAGE;
    for (const char *from = argv[1], *to = argv[2]; *from; ++from, ++to) {
        table[(unsigned)*from] = *to;
    }

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr");

    struct winsize window;
    error = ioctl(STDERR_FILENO, TIOCGWINSZ, &window);
    if (error) err(EX_IOERR, "ioctl(%d, TIOCGWINSZ, ...)", STDERR_FILENO);

    int pty;
    pid_t pid = forkpty(&pty, NULL, NULL, &window);
    if (pid < 0) err(EX_OSERR, "forkpty");

    if (!pid) {
        execvp(argv[3], &argv[3]);
        err(EX_NOPERM, "%s", argv[3]);
    }

    bool enable = true;

    char buf[4096];
    struct pollfd fds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = pty, .events = POLLIN },
    };
    while (0 < poll(fds, 2, -1)) {
        if (fds[0].revents == POLLIN) {
            ssize_t readSize = read(STDIN_FILENO, buf, sizeof(buf));
            if (readSize < 0) err(EX_IOERR, "read(%d)", STDIN_FILENO);

            if (readSize == 1) {
                if (buf[0] == CTRL('S')) {
                    enable ^= true;
                    continue;
                }

                unsigned char c = buf[0];
                if (enable && table[c]) buf[0] = table[c];
            }

            ssize_t writeSize = write(pty, buf, readSize);
            if (writeSize < 0) err(EX_IOERR, "write(%d)", pty);
            if (writeSize < readSize) errx(EX_IOERR, "short write(%d)", pty);
        }

        if (fds[1].revents == POLLIN) {
            ssize_t readSize = read(pty, buf, sizeof(buf));
            if (readSize < 0) err(EX_IOERR, "read(%d)", pty);

            ssize_t writeSize = write(STDOUT_FILENO, buf, readSize);
            if (writeSize < 0) err(EX_IOERR, "write(%d)", STDOUT_FILENO);
            if (writeSize < readSize) {
                errx(EX_IOERR, "short write(%d)", STDOUT_FILENO);
            }
        }

        int status;
        pid_t dead = waitpid(pid, &status, WNOHANG);
        if (dead < 0) err(EX_OSERR, "waitpid(%d, ...)", pid);
        if (dead) return WIFEXITED(status) ? WEXITSTATUS(status) : EX_SOFTWARE;
    }
    err(EX_IOERR, "poll");
}
