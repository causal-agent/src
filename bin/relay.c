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
 *
 * Additional permission under GNU GPL version 3 section 7:
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with LibreSSL (or a modified version of that library),
 * containing parts covered by the terms of the OpenSSL License and the
 * original SSLeay license, the licensors of this Program grant you
 * additional permission to convey the resulting work. Corresponding
 * Source for a non-source form of such a combination shall include the
 * source code for the parts of LibreSSL used as well as that of the
 * covered work.
 */

#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sysexits.h>
#include <tls.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/capsicum.h>
#endif

static void clientWrite(struct tls *client, const char *ptr, size_t len) {
	while (len) {
		ssize_t ret = tls_write(client, ptr, len);
		if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) continue;
		if (ret < 0) errx(EX_IOERR, "tls_write: %s", tls_error(client));
		ptr += ret;
		len -= ret;
	}
}

static void clientFormat(struct tls *client, const char *format, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	int len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	if ((size_t)len > sizeof(buf) - 1) errx(EX_DATAERR, "message too large");
	clientWrite(client, buf, len);
}

static void clientHandle(struct tls *client, const char *chan, char *line) {
	char *prefix = NULL;
	if (line[0] == ':') {
		prefix = strsep(&line, " ") + 1;
		if (!line) errx(EX_PROTOCOL, "unexpected eol");
	}

	char *command = strsep(&line, " ");
	if (!strcmp(command, "001") || !strcmp(command, "INVITE")) {
		clientFormat(client, "JOIN :%s\r\n", chan);
	} else if (!strcmp(command, "PING")) {
		clientFormat(client, "PONG %s\r\n", line);
	}
	if (strcmp(command, "PRIVMSG") && strcmp(command, "NOTICE")) return;

	if (!prefix) errx(EX_PROTOCOL, "message without prefix");
	char *nick = strsep(&prefix, "!");

	if (!line) errx(EX_PROTOCOL, "message without destination");
	char *dest = strsep(&line, " ");
	if (strcmp(dest, chan)) return;

	if (!line || line[0] != ':') errx(EX_PROTOCOL, "message without message");
	line = &line[1];

	if (!strncmp(line, "\1ACTION ", 8)) {
		line = &line[8];
		size_t len = strcspn(line, "\1");
		printf("* %c\u200C%s %.*s\n", nick[0], &nick[1], (int)len, line);
	} else if (command[0] == 'N') {
		printf("-%c\u200C%s- %s\n", nick[0], &nick[1], line);
	} else {
		printf("<%c\u200C%s> %s\n", nick[0], &nick[1], line);
	}
}

#ifdef __FreeBSD__
static void limit(int fd, const cap_rights_t *rights) {
	int error = cap_rights_limit(fd, rights);
	if (error) err(EX_OSERR, "cap_rights_limit");
}
#endif

int main(int argc, char *argv[]) {
	int error;

	if (argc < 5) return EX_USAGE;
	const char *host = argv[1];
	const char *port = argv[2];
	const char *nick = argv[3];
	const char *chan = argv[4];

	setlinebuf(stdout);
	signal(SIGPIPE, SIG_IGN);

	struct tls_config *config = tls_config_new();
	if (!config) errx(EX_SOFTWARE, "tls_config_new");

	error = tls_config_set_ciphers(config, "compat");
	if (error) {
		errx(EX_SOFTWARE, "tls_config_set_ciphers: %s", tls_config_error(config));
	}

	struct tls *client = tls_client();
	if (!client) errx(EX_SOFTWARE, "tls_client");

	error = tls_configure(client, config);
	if (error) errx(EX_SOFTWARE, "tls_configure: %s", tls_error(client));
	tls_config_free(config);

	struct addrinfo *head;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP,
	};
	error = getaddrinfo(host, port, &hints, &head);
	if (error) errx(EX_NOHOST, "getaddrinfo: %s", gai_strerror(error));

	int sock = -1;
	for (struct addrinfo *ai = head; ai; ai = ai->ai_next) {
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock < 0) err(EX_OSERR, "socket");

		error = connect(sock, ai->ai_addr, ai->ai_addrlen);
		if (!error) break;

		close(sock);
		sock = -1;
	}
	if (sock < 0) err(EX_UNAVAILABLE, "connect");
	freeaddrinfo(head);

	error = tls_connect_socket(client, sock, host);
	if (error) errx(EX_PROTOCOL, "tls_connect: %s", tls_error(client));

#ifdef __FreeBSD__
	error = cap_enter();
	if (error) err(EX_OSERR, "cap_enter");

	cap_rights_t rights;
	cap_rights_init(&rights, CAP_WRITE);
	limit(STDOUT_FILENO, &rights);
	limit(STDERR_FILENO, &rights);

	cap_rights_init(&rights, CAP_EVENT, CAP_READ);
	limit(STDIN_FILENO, &rights);

	cap_rights_set(&rights, CAP_WRITE);
	limit(sock, &rights);
#endif

	clientFormat(client, "NICK :%s\r\nUSER %s 0 * :%s\r\n", nick, nick, nick);

	char *input = NULL;
	size_t cap = 0;

	char buf[4096];
	size_t len = 0;

	struct pollfd fds[2] = {
		{ .events = POLLIN, .fd = STDIN_FILENO },
		{ .events = POLLIN, .fd = sock },
	};
	while (0 < poll(fds, 2, -1)) {
		if (fds[0].revents) {
			ssize_t len = getline(&input, &cap, stdin);
			if (len < 0) err(EX_IOERR, "getline");
			input[len - 1] = '\0';
			clientFormat(client, "NOTICE %s :%s\r\n", chan, input);
		}
		if (!fds[1].revents) continue;

		ssize_t read = tls_read(client, &buf[len], sizeof(buf) - len);
		if (read == TLS_WANT_POLLIN || read == TLS_WANT_POLLOUT) continue;
		if (read < 0) errx(EX_IOERR, "tls_read: %s", tls_error(client));
		if (!read) return EX_UNAVAILABLE;
		len += read;

		char *crlf;
		char *line = buf;
		for (;;) {
			crlf = memmem(line, &buf[len] - line, "\r\n", 2);
			if (!crlf) break;
			crlf[0] = '\0';
			clientHandle(client, chan, line);
			line = &crlf[2];
		}
		len -= line - buf;
		memmove(buf, line, len);
	}
	err(EX_IOERR, "poll");
}
