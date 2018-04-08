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

#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

static void watch(int kq, char *path) {
    int fd = open(path, O_CLOEXEC);
    if (fd < 0) err(EX_NOINPUT, "%s", path);

    struct kevent event = {
        .ident = fd,
        .filter = EVFILT_VNODE,
        .flags = EV_ADD | EV_CLEAR,
        .fflags = NOTE_WRITE | NOTE_DELETE,
        .udata = path,
    };
    int nevents = kevent(kq, &event, 1, NULL, 0, NULL);
    if (nevents < 0) err(EX_OSERR, "kevent");
}

static void exec(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) err(EX_OSERR, "fork");

    if (!pid) {
        execvp(*argv, argv);
        err(EX_NOINPUT, "%s", *argv);
    }

    int status;
    pid = wait(&status);
    if (pid < 0) err(EX_OSERR, "wait");

    if (WIFEXITED(status)) {
        warnx("exit %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        warnx("signal %d\n", WTERMSIG(status));
    } else {
        warnx("status %d\n", status);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) return EX_USAGE;

    int kq = kqueue();
    if (kq < 0) err(EX_OSERR, "kqueue");

    int i;
    for (i = 1; i < argc - 1; ++i) {
        if (argv[i][0] == '-') {
            i++;
            break;
        }
        watch(kq, argv[i]);
    }

    exec(&argv[i]);

    for (;;) {
        struct kevent event;
        int nevents = kevent(kq, NULL, 0, &event, 1, NULL);
        if (nevents < 0) err(EX_OSERR, "kevent");

        if (event.fflags & NOTE_DELETE) {
            close(event.ident);
            sleep(1);
            watch(kq, event.udata);
        }

        exec(&argv[i]);
    }
}
