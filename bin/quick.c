/* Copyright (C) 2021  June McEnroe <june@causal.agency>
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
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

static void request(int sock, char *argv[]) {
	struct pollfd pfd = { .fd = sock, .events = POLLIN };
	int nfds = poll(&pfd, 1, -1);
	if (nfds < 0) err(EX_OSERR, "poll");

	char buf[4096];
	ssize_t len = recv(sock, buf, sizeof(buf)-1, MSG_PEEK);
	if (len < 0) {
		warn("recv");
		return;
	}
	char *blank = memmem(buf, len, "\r\n\r\n", 4);
	if (!blank) {
		warnx("can't find end of request headers in peek");
		return;
	}
	len = recv(sock, buf, &blank[4] - buf, 0);
	if (len < 0) {
		warn("recv");
		return;
	}
	buf[len] = '\0';

	char *ptr = buf;
	char *req = strsep(&ptr, "\r\n");
	char *method = strsep(&req, " ");
	char *query = strsep(&req, " ");
	char *path = strsep(&query, "?");
	char *proto = strsep(&req, " ");
	if (!method || !path || !proto) {
		warnx("invalid request line");
		return;
	}
	setenv("REQUEST_METHOD", method, 1);
	setenv("PATH_INFO", path, 1);
	setenv("QUERY_STRING", (query ? query : ""), 1);
	setenv("SERVER_PROTOCOL", proto, 1);

	unsetenv("CONTENT_TYPE");
	unsetenv("CONTENT_LENGTH");
	unsetenv("HTTP_HOST");
	while (ptr) {
		char *value = strsep(&ptr, "\r\n");
		if (!value[0]) continue;
		char *header = strsep(&value, ":");
		if (!header || !value++) {
			warnx("invalid header");
			return;
		}
		if (!strcasecmp(header, "Content-Type")) {
			setenv("CONTENT_TYPE", value, 1);
		} else if (!strcasecmp(header, "Content-Length")) {
			setenv("CONTENT_LENGTH", value, 1);
		} else if (!strcasecmp(header, "Host")) {
			setenv("HTTP_HOST", value, 1);
		}
	}

	dprintf(sock, "HTTP/1.1 200 OK\nConnection: close\n");
	pid_t pid = fork();
	if (pid < 0) err(EX_OSERR, "fork");
	if (!pid) {
		dup2(sock, STDIN_FILENO);
		dup2(sock, STDOUT_FILENO);
		execv(argv[0], argv);
		warn("%s", argv[0]);
		_exit(127);
	}

	int status;
	pid = wait(&status);
	if (pid < 0) err(EX_OSERR, "wait");
	if (WIFEXITED(status) && WEXITSTATUS(status)) {
		warnx("%s exited %d", argv[0], WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		warnx("%s killed %d", argv[0], WTERMSIG(status));
	}
}

int main(int argc, char *argv[]) {
	short port = 0;
	for (int opt; 0 < (opt = getopt(argc, argv, "p:"));) {
		switch (opt) {
			break; case 'p': port = atoi(optarg);
			break; default:  return EX_USAGE;
		}
	}
	if (optind == argc) errx(EX_USAGE, "script required");

	int server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0) err(EX_OSERR, "socket");
	fcntl(server, F_SETFD, FD_CLOEXEC);

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = htonl(INADDR_LOOPBACK),
	};
	socklen_t addrlen = sizeof(addr);
	int error = 0
		|| bind(server, (struct sockaddr *)&addr, addrlen)
		|| getsockname(server, (struct sockaddr *)&addr, &addrlen)
		|| listen(server, -1);
	if (error) err(EX_UNAVAILABLE, "%hd", port);

	char host[NI_MAXHOST], serv[NI_MAXSERV];
	error = getnameinfo(
		(struct sockaddr *)&addr, addrlen,
		host, sizeof(host), serv, sizeof(serv),
		NI_NOFQDN | NI_NUMERICSERV
	);
	if (error) errx(EX_UNAVAILABLE, "getnameinfo: %s", gai_strerror(error));
	printf("http://%s:%s/\n", host, serv);
	fflush(stdout);

	setenv("SERVER_SOFTWARE", "quick (and dirty)", 1);
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
	setenv("SERVER_NAME", host, 1);
	setenv("SERVER_PORT", serv, 1);
	setenv("REMOTE_ADDR", "127.0.0.1", 1);
	setenv("REMOTE_HOST", host, 1);
	setenv("SCRIPT_NAME", "/", 1);

	for (int sock; 0 <= (sock = accept(server, NULL, NULL)); close(sock)) {
		request(sock, &argv[optind]);
	}
	err(EX_IOERR, "accept");
}
