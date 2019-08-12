/* Copyright (C) 2017-2019  June McEnroe <june@causal.agency>
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
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
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

static char _;
static struct iovec iov = { .iov_base = &_, .iov_len = 1 };

static ssize_t sendfd(int sock, int fd) {
	size_t len = CMSG_SPACE(sizeof(int));
	char buf[len];
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = buf,
		.msg_controllen = len,
	};

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*(int *)CMSG_DATA(cmsg) = fd;

	return sendmsg(sock, &msg, 0);
}

static int recvfd(int sock) {
	size_t len = CMSG_SPACE(sizeof(int));
	char buf[len];
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = buf,
		.msg_controllen = len,
	};
	if (0 > recvmsg(sock, &msg, 0)) return -1;

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS) {
		errno = ENOMSG;
		return -1;
	}
	return *(int *)CMSG_DATA(cmsg);
}

static struct sockaddr_un addr = { .sun_family = AF_UNIX };

static void handler(int sig) {
	unlink(addr.sun_path);
	_exit(-sig);
}

static void detach(int server, bool sink, char *argv[]) {
	int pty;
	pid_t pid = forkpty(&pty, NULL, NULL, NULL);
	if (pid < 0) err(EX_OSERR, "forkpty");

	if (!pid) {
		execvp(argv[0], argv);
		err(EX_NOINPUT, "%s", argv[0]);
	}

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	int error = listen(server, 0);
	if (error) err(EX_OSERR, "listen");

	struct pollfd fds[] = {
		{ .events = POLLIN, .fd = server },
		{ .events = POLLIN, .fd = pty },
	};
	while (0 < poll(fds, (sink ? 2 : 1), -1)) {
		if (fds[0].revents) {
			int client = accept(server, NULL, NULL);
			if (client < 0) err(EX_IOERR, "accept");

			ssize_t len = sendfd(client, pty);
			if (len < 0) warn("sendfd");

			len = recv(client, &_, sizeof(_), 0);
			if (len < 0) warn("recv");

			close(client);
		}

		if (fds[1].revents) {
			char buf[4096];
			ssize_t len = read(pty, buf, sizeof(buf));
			if (len < 0) err(EX_IOERR, "read");
		}

		int status;
		pid_t dead = waitpid(pid, &status, WNOHANG);
		if (dead < 0) err(EX_OSERR, "waitpid");
		if (dead) {
			unlink(addr.sun_path);
			exit(WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status));
		}
	}
	err(EX_IOERR, "poll");
}

static struct termios saveTerm;
static void restoreTerm(void) {
	tcsetattr(STDIN_FILENO, TCSADRAIN, &saveTerm);
	fprintf(stderr, "\33c"); // RIS
	warnx("detached");
}

static void nop(int sig) {
	(void)sig;
}

static void attach(int client) {
	int error;

	int pty = recvfd(client);
	if (pty < 0) err(EX_IOERR, "recvfd");
	warnx("attached");

	struct winsize window;
	error = ioctl(STDIN_FILENO, TIOCGWINSZ, &window);
	if (error) err(EX_IOERR, "ioctl");

	struct winsize redraw = { .ws_row = 1, .ws_col = 1 };
	error = ioctl(pty, TIOCSWINSZ, &redraw);
	if (error) err(EX_IOERR, "ioctl");

	error = ioctl(pty, TIOCSWINSZ, &window);
	if (error) err(EX_IOERR, "ioctl");

	error = tcgetattr(STDIN_FILENO, &saveTerm);
	if (error) err(EX_IOERR, "tcgetattr");
	atexit(restoreTerm);

	struct termios raw = saveTerm;
	cfmakeraw(&raw);
	error = tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
	if (error) err(EX_IOERR, "tcsetattr");

	signal(SIGWINCH, nop);

	char buf[4096];
	struct pollfd fds[] = {
		{ .events = POLLIN, .fd = STDIN_FILENO },
		{ .events = POLLIN, .fd = pty },
	};
	for (;;) {
		int nfds = poll(fds, 2, -1);
		if (nfds < 0) {
			if (errno != EINTR) err(EX_IOERR, "poll");

			error = ioctl(STDIN_FILENO, TIOCGWINSZ, &window);
			if (error) err(EX_IOERR, "ioctl");

			error = ioctl(pty, TIOCSWINSZ, &window);
			if (error) err(EX_IOERR, "ioctl");

			continue;
		}

		if (fds[0].revents) {
			ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
			if (len < 0) err(EX_IOERR, "read");
			if (!len) break;

			if (len == 1 && buf[0] == CTRL('Q')) break;

			len = write(pty, buf, len);
			if (len < 0) err(EX_IOERR, "write");
		}

		if (fds[1].revents) {
			ssize_t len = read(pty, buf, sizeof(buf));
			if (len < 0) err(EX_IOERR, "read");
			if (!len) break;

			len = write(STDOUT_FILENO, buf, len);
			if (len < 0) err(EX_IOERR, "write");
		}
	}
}

int main(int argc, char *argv[]) {
	int error;

	bool atch = false;
	bool sink = false;

	int opt;
	while (0 < (opt = getopt(argc, argv, "as"))) {
		switch (opt) {
			break; case 'a': atch = true;
			break; case 's': sink = true;
			break; default:  return EX_USAGE;
		}
	}
	if (optind == argc) errx(EX_USAGE, "no session name");
	const char *name = argv[optind++];

	if (optind == argc) {
		argv[--optind] = getenv("SHELL");
		if (!argv[optind]) errx(EX_CONFIG, "SHELL unset");
	}

	const char *home = getenv("HOME");
	if (!home) errx(EX_CONFIG, "HOME unset");

	int fd = open(home, 0);
	if (fd < 0) err(EX_CANTCREAT, "%s", home);

	error = mkdirat(fd, ".dtch", 0700);
	if (error && errno != EEXIST) err(EX_CANTCREAT, "%s/.dtch", home);

	close(fd);

	int sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) err(EX_OSERR, "socket");
	fcntl(sock, F_SETFD, FD_CLOEXEC);

	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/.dtch/%s", home, name);

	if (atch) {
		error = connect(sock, (struct sockaddr *)&addr, SUN_LEN(&addr));
		if (error) err(EX_NOINPUT, "%s", addr.sun_path);
		attach(sock);
	} else {
		error = bind(sock, (struct sockaddr *)&addr, SUN_LEN(&addr));
		if (error) err(EX_CANTCREAT, "%s", addr.sun_path);
		detach(sock, sink, &argv[optind]);
	}
}
