/* Copyright (C) 2018  June McEnroe <june@causal.agency>
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

typedef unsigned uint;
typedef unsigned char byte;

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
	int error;

	uint baudRate = 19200;
	for (int opt; 0 < (opt = getopt(argc, argv, "r:"));) {
		switch (opt) {
			break; case 'r': baudRate = strtoul(optarg, NULL, 10);
			break; default:  return EX_USAGE;
		}
	}
	if (argc - optind < 1) return EX_USAGE;

	error = tcgetattr(STDIN_FILENO, &saveTerm);
	if (error) err(EX_IOERR, "tcgetattr");
	atexit(restoreTerm);

	struct termios raw = saveTerm;
	cfmakeraw(&raw);
	error = tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
	if (error) err(EX_IOERR, "tcsetattr");

	struct winsize window;
	error = ioctl(STDIN_FILENO, TIOCGWINSZ, &window);
	if (error) err(EX_IOERR, "TIOCGWINSZ");

	int pty;
	pid_t pid = forkpty(&pty, NULL, NULL, &window);
	if (pid < 0) err(EX_OSERR, "forkpty");

	if (!pid) {
		execvp(argv[optind], &argv[optind]);
		err(EX_NOINPUT, "%s", argv[optind]);
	}

	byte c;
	struct pollfd fds[2] = {
		{ .events = POLLIN, .fd = STDIN_FILENO },
		{ .events = POLLIN, .fd = pty },
	};
	while (usleep(8 * 1000000 / baudRate), 0 < poll(fds, 2, -1)) {
		if (fds[0].revents) {
			ssize_t size = read(STDIN_FILENO, &c, 1);
			if (size < 0) err(EX_IOERR, "read(%d)", STDIN_FILENO);
			size = write(pty, &c, 1);
			if (size < 0) err(EX_IOERR, "write(%d)", pty);
		}

		if (fds[1].revents) {
			ssize_t size = read(pty, &c, 1);
			if (size < 0) err(EX_IOERR, "read(%d)", pty);
			if (!size) break;
			size = write(STDOUT_FILENO, &c, 1);
			if (size < 0) err(EX_IOERR, "write(%d)", STDOUT_FILENO);
		}
	}

	int status;
	pid_t dead = waitpid(pid, &status, 0);
	if (dead < 0) err(EX_OSERR, "waitpid");
	return WIFEXITED(status) ? WEXITSTATUS(status) : EX_SOFTWARE;
}
