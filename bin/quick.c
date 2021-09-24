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
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

static void request(int sock, char *argv[]) {
	FILE *req = fdopen(dup(sock), "r");
	if (!req) err(EX_OSERR, "fdopen");
	fcntl(fileno(req), F_SETFD, FD_CLOEXEC);

	size_t cap = 0;
	char *buf = NULL;
	ssize_t len = getline(&buf, &cap, req);
	if (len < 0) goto close;

	char *ptr = buf;
	char *method = strsep(&ptr, " ");
	char *query = strsep(&ptr, " ");
	char *path = strsep(&query, "?");
	char *proto = strsep(&ptr, "\r\n");
	if (!method || !path || !proto) goto close;

	setenv("REQUEST_METHOD", method, 1);
	setenv("PATH_INFO", path, 1);
	setenv("QUERY_STRING", (query ? query : ""), 1);
	setenv("SERVER_PROTOCOL", proto, 1);
	unsetenv("CONTENT_TYPE");
	unsetenv("CONTENT_LENGTH");
	unsetenv("HTTP_HOST");

	size_t bodyLen = 0;
	while (0 <= (len = getline(&buf, &cap, req))) {
		if (len && buf[len-1] == '\n') buf[--len] = '\0';
		if (len && buf[len-1] == '\r') buf[--len] = '\0';
		if (!len) break;

		char *value = buf;
		char *header = strsep(&value, ":");
		if (!header || !value++) goto close;

		if (!strcasecmp(header, "Content-Type")) {
			setenv("CONTENT_TYPE", value, 1);
		} else if (!strcasecmp(header, "Content-Length")) {
			bodyLen = strtoull(value, NULL, 10);
			setenv("CONTENT_LENGTH", value, 1);
		} else {
			char buf[256];
			for (char *ch = header; *ch; ++ch) {
				*ch = (*ch == '-' ? '_' : toupper(*ch));
			}
			snprintf(buf, sizeof(buf), "HTTP_%s", header);
			setenv(buf, value, 1);
		}
	}

	int rw[2];
	int error = pipe(rw);
	if (error) err(EX_OSERR, "pipe");
	fcntl(rw[0], F_SETFD, FD_CLOEXEC);
	fcntl(rw[1], F_SETFD, FD_CLOEXEC);

	dprintf(sock, "HTTP/1.1 200 OK\nConnection: close\n");
	pid_t pid = fork();
	if (pid < 0) err(EX_OSERR, "fork");
	if (!pid) {
		dup2(rw[0], STDIN_FILENO);
		dup2(sock, STDOUT_FILENO);
		execv(argv[0], argv);
		warn("%s", argv[0]);
		_exit(127);
	}

	close(rw[0]);
	char body[4096];
	while (bodyLen) {
		size_t cap = (bodyLen < sizeof(body) ? bodyLen : sizeof(body));
		size_t len = fread(&body, 1, cap, req);
		if (!len) break;
		write(rw[1], body, len);
		bodyLen -= len;
	}
	close(rw[1]);

	int status;
	pid = wait(&status);
	if (pid < 0) err(EX_OSERR, "wait");
	if (WIFEXITED(status) && WEXITSTATUS(status)) {
		warnx("%s exited %d", argv[0], WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		warnx("%s killed %d", argv[0], WTERMSIG(status));
	}

close:
	fclose(req);
	free(buf);
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

	signal(SIGPIPE, SIG_IGN);
	for (int sock; 0 <= (sock = accept(server, NULL, NULL)); close(sock)) {
		request(sock, &argv[optind]);
	}
	err(EX_IOERR, "accept");
}
