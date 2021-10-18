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
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef unsigned char byte;

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
	if (argc < 2) return EX_USAGE;
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
		execvp(argv[1], &argv[1]);
		err(EX_NOINPUT, "%s", argv[1]);
	}

	bool stop = false;

	byte buf[4096];
	struct pollfd fds[2] = {
		{ .events = POLLIN, .fd = STDIN_FILENO },
		{ .events = POLLIN, .fd = pty },
	};
	while (0 < poll(fds, 2, -1)) {
		if (fds[0].revents & POLLIN) {
			ssize_t rlen = read(STDIN_FILENO, buf, sizeof(buf));
			if (rlen < 0) err(EX_IOERR, "read");

			if (rlen == 1 && buf[0] == CTRL('Q')) {
				stop ^= true;
				continue;
			}

			if (rlen == 1 && buf[0] == CTRL('S')) {
				char dump[] = "\x1B[10i";
				ssize_t wlen = write(STDOUT_FILENO, dump, sizeof(dump) - 1);
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
	err(EX_IOERR, "poll");
}
