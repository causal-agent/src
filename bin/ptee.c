/* Copyright (C) 2019  June McEnroe <june@causal.agency>
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
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
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

typedef unsigned char byte;

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
}

static void handler(int sig) {
	(void)sig;
}

int main(int argc, char *argv[]) {
	int timer = 0;
	for (int opt; 0 < (opt = getopt(argc, argv, "t:"));) {
		switch (opt) {
			break; case 't': timer = atoi(optarg);
			break; default:  return EX_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) return EX_USAGE;
	if (isatty(STDOUT_FILENO)) errx(EX_USAGE, "stdout is not redirected");

	int error = tcgetattr(STDIN_FILENO, &saveTerm);
	if (error) err(EX_IOERR, "tcgetattr");
	atexit(restoreTerm);

	struct termios raw = saveTerm;
	cfmakeraw(&raw);
	error = tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
	if (error) err(EX_IOERR, "tcsetattr");

	struct winsize window;
	error = ioctl(STDIN_FILENO, TIOCGWINSZ, &window);
	if (error) err(EX_IOERR, "ioctl");

	int pty;
	pid_t pid = forkpty(&pty, NULL, NULL, &window);
	if (pid < 0) err(EX_OSERR, "forkpty");

	if (!pid) {
		execvp(argv[0], argv);
		err(EX_NOINPUT, "%s", argv[0]);
	}

	if (timer) {
		signal(SIGALRM, handler);
		struct timeval tv = {
			.tv_sec = timer / 1000,
			.tv_usec = timer % 1000 * 1000,
		};
		struct itimerval itv = { tv, tv };
		setitimer(ITIMER_REAL, &itv, NULL);
	}

	char mc[] = "\x1B[10i";
	bool stop = false;

	byte buf[4096];
	struct pollfd fds[2] = {
		{ .events = POLLIN, .fd = STDIN_FILENO },
		{ .events = POLLIN, .fd = pty },
	};
	for (;;) {
		int nfds = poll(fds, 2, -1);
		if (nfds < 0 && errno != EINTR) err(EX_IOERR, "poll");

		if (nfds < 0) {
			ssize_t wlen = write(STDOUT_FILENO, mc, sizeof(mc) - 1);
			if (wlen < 0) err(EX_IOERR, "write");
			continue;
		}

		if (fds[0].revents & POLLIN) {
			ssize_t rlen = read(STDIN_FILENO, buf, sizeof(buf));
			if (rlen < 0) err(EX_IOERR, "read");

			if (rlen == 1 && buf[0] == CTRL('Q')) {
				stop ^= true;
				continue;
			}

			if (rlen == 1 && buf[0] == CTRL('S')) {
				ssize_t wlen = write(STDOUT_FILENO, mc, sizeof(mc) - 1);
				if (wlen < 0) err(EX_IOERR, "write");
				continue;
			}

			ssize_t wlen = write(pty, buf, rlen);
			if (wlen < 0) err(EX_IOERR, "write");
		}

		if (fds[1].revents & POLLIN) {
			ssize_t rlen = read(pty, buf, sizeof(buf));
			if (rlen < 0) err(EX_IOERR, "read");

			ssize_t wlen = write(STDIN_FILENO, buf, rlen);
			if (wlen < 0) err(EX_IOERR, "write");

			if (!stop) {
				wlen = write(STDOUT_FILENO, buf, rlen);
				if (wlen < 0) err(EX_IOERR, "write");
			}
		}

		int status;
		pid_t dead = waitpid(pid, &status, WNOHANG);
		if (dead < 0) err(EX_OSERR, "waitpid");
		if (dead) return WIFEXITED(status) ? WEXITSTATUS(status) : EX_SOFTWARE;
	}
}
