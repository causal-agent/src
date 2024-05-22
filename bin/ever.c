/* Copyright (C) 2017  June McEnroe <june@causal.agency>
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
#include <stdbool.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/wait.h>
#include <unistd.h>

static int watch(int kq, char *path) {
	int fd = open(path, O_CLOEXEC);
	if (fd < 0) err(1, "%s", path);

	struct kevent event;
	EV_SET(
		&event,
		fd,
		EVFILT_VNODE,
		EV_ADD | EV_CLEAR,
		NOTE_WRITE | NOTE_DELETE,
		0,
		path
	);
	int nevents = kevent(kq, &event, 1, NULL, 0, NULL);
	if (nevents < 0) err(1, "kevent");

	return fd;
}

static bool quiet;
static void exec(int fd, char *const argv[]) {
	pid_t pid = fork();
	if (pid < 0) err(1, "fork");

	if (!pid) {
		dup2(fd, STDIN_FILENO);
		execvp(*argv, argv);
		err(127, "%s", *argv);
	}

	int status;
	pid = wait(&status);
	if (pid < 0) err(1, "wait");

	if (quiet) return;
	if (WIFEXITED(status)) {
		warnx("exit %d\n", WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		warnx("signal %d\n", WTERMSIG(status));
	} else {
		warnx("status %d\n", status);
	}
}

int main(int argc, char *argv[]) {
	bool input = false;

	for (int opt; 0 < (opt = getopt(argc, argv, "iq"));) {
		switch (opt) {
			break; case 'i': input = true;
			break; case 'q': quiet = true;
			break; default:  return 1;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 2) return 1;

	int kq = kqueue();
	if (kq < 0) err(1, "kqueue");

	int i;
	for (i = 0; i < argc - 1; ++i) {
		if (argv[i][0] == '-') {
			i++;
			break;
		}
		watch(kq, argv[i]);
	}

	if (!input) {
		exec(STDIN_FILENO, &argv[i]);
	}

	for (;;) {
		struct kevent event;
		int nevents = kevent(kq, NULL, 0, &event, 1, NULL);
		if (nevents < 0) err(1, "kevent");

		if (event.fflags & NOTE_DELETE) {
			close(event.ident);
			sleep(1);
			event.ident = watch(kq, (char *)event.udata);
		} else if (input) {
			off_t off = lseek(event.ident, 0, SEEK_SET);
			if (off < 0) err(1, "lseek");
		}

		exec((input ? event.ident : STDIN_FILENO), &argv[i]);
	}
}
