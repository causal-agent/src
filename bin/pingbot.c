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
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned uint;

__attribute__((format(printf, 2, 3)))
static int pdprintf(int fd, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	va_start(ap, format);
	int len = vdprintf(fd, format, ap);
	va_end(ap);
	return len;
}

int main(int argc, char *argv[]) {
	int error;

	const char *host = NULL;
	const char *port = "6667";
	const char *nick = "pingbot";
	const char *join = NULL;
	uint timeout = 120;
	uint delay = 30;

	int opt;
	while (0 < (opt = getopt(argc, argv, "d:j:n:p:t:"))) {
		switch (opt) {
			break; case 'd': delay = strtoul(optarg, NULL, 0);
			break; case 'j': join = optarg;
			break; case 'n': nick = optarg;
			break; case 'p': port = optarg;
			break; case 't': timeout = strtoul(optarg, NULL, 0);
			break; default:  return EX_USAGE;
		}
	}
	if (optind < argc) host = argv[optind];
	if (!host || !timeout) return EX_USAGE;

	struct addrinfo *head;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	error = getaddrinfo(host, port, &hints, &head);
	if (error) errx(EX_NOHOST, "getaddrinfo: %s", gai_strerror(error));

	int client = -1;
	struct addrinfo *ai;
	for (ai = head; ai; ai = ai->ai_next) {
		client = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (client < 0) err(EX_OSERR, "socket");

		error = connect(client, ai->ai_addr, ai->ai_addrlen);
		if (!error) break;

		close(client);
		client = -1;
	}
	if (client < 0) err(EX_UNAVAILABLE, "connect");

	error = pdprintf(client, "NICK %1$s\r\nUSER %1$s 0 * :%1$s\r\n", nick);
	if (error < 0) err(EX_IOERR, "pdprintf");

	int kq = kqueue();
	if (kq < 0) err(EX_OSERR, "kqueue");

	struct kevent event;
	EV_SET(&event, client, EVFILT_READ, EV_ADD, 0, 0, 0);
	int nevents = kevent(kq, &event, 1, NULL, 0, NULL);
	if (nevents < 0) err(EX_OSERR, "kevent");

	char buf[4096];
	size_t len = 0;
	bool pong = true;
	for (;;) {
		nevents = kevent(kq, NULL, 0, &event, 1, NULL);
		if (nevents < 0) err(EX_IOERR, "kevent");

		if (event.filter == EVFILT_TIMER) {
			char *ping = event.udata;
			if (pong) {
				error = pdprintf(client, "PONG %s\r\n", ping);
				if (error < 0) err(EX_IOERR, "pdprintf");
			}
			pong = true;
			free(ping);
			continue;
		}

		ssize_t rlen = read(client, &buf[len], sizeof(buf) - len);
		if (rlen < 0) err(EX_IOERR, "read");
		len += (size_t)rlen;

		char *crlf;
		char *line = buf;
		while (NULL != (crlf = strnstr(line, "\r\n", &buf[len] - line))) {
			crlf[0] = '\0';
			printf("%s\n", line);

			if (line[0] == ':') strsep(&line, " ");
			if (!line) errx(EX_PROTOCOL, "unexpected eol");
			char *cmd = strsep(&line, " ");

			if (!strcmp(cmd, "001") && join) {
				error = pdprintf(client, "JOIN :%s\r\n", join);
				if (error < 0) err(EX_IOERR, "pdprintf");

			} else if (!strcmp(cmd, "PING")) {
				EV_SET(
					&event, 0, EVFILT_TIMER, EV_ADD | EV_ONESHOT,
					NOTE_SECONDS, timeout - 1, strdup(line)
				);
				nevents = kevent(kq, &event, 1, NULL, 0, NULL);
				if (nevents < 0) err(EX_OSERR, "kevent");

			} else if (!strcmp(cmd, "PRIVMSG") && strcasestr(line, nick)) {
				pong = false;

			} else if (!strcmp(cmd, "ERROR")) {
				close(client);
				sleep(delay);

				client = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
				if (client < 0) err(EX_OSERR, "socket");

				error = connect(client, ai->ai_addr, ai->ai_addrlen);
				if (error) err(EX_UNAVAILABLE, "connect");

				error = pdprintf(
					client, "NICK %1$s\r\nUSER %1$s 0 * :%1$s\r\n", nick
				);
				if (error < 0) err(EX_IOERR, "pdprintf");

				EV_SET(&event, client, EVFILT_READ, EV_ADD, 0, 0, 0);
				int nevents = kevent(kq, &event, 1, NULL, 0, NULL);
				if (nevents < 0) err(EX_OSERR, "kevent");
			}

			line = &crlf[2];
		}

		len -= line - buf;
		memmove(buf, line, len);
	}
}
