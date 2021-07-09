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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <tls.h>
#include <unistd.h>

static bool verbose;
static struct tls *client;
static const char *join;
static bool reverse;

static void clientWrite(const char *ptr, size_t len) {
	if (verbose) printf("%.*s", (int)len, ptr);
	while (len) {
		ssize_t ret = tls_write(client, ptr, len);
		if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) continue;
		if (ret < 0) errx(EX_IOERR, "tls_write: %s", tls_error(client));
		ptr += ret;
		len -= ret;
	}
}

static void format(const char *format, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	int len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	if ((size_t)len > sizeof(buf) - 1) errx(EX_DATAERR, "message too large");
	clientWrite(buf, len);
}

static void handle(char *line) {
	char *tags = NULL;
	char *origin = NULL;
	if (line && line[0] == '@') tags = 1 + strsep(&line, " ");
	if (line && line[0] == ':') origin = 1 + strsep(&line, " ");
	char *cmd = strsep(&line, " ");
	if (!cmd) return;
	if (!strcmp(cmd, "CAP")) {
		char *param = strsep(&line, " ");
		if (!param) errx(EX_PROTOCOL, "CAP missing parameter");
		if (!strcmp(param, "NAK")) {
			errx(EX_CONFIG, "server does not support %s", line);
		}
		format("CAP END\r\n");
	} else if (!strcmp(cmd, "001") && join) {
		format("JOIN %s\r\n", join);
	} else if (!strcmp(cmd, "PING")) {
		format("PONG %s\r\n", line);
	}
	if (strcmp(cmd, "TAGMSG") || !tags || !origin) return;
	if (!strstr(tags, "typing=")) return;
	char *nick = strsep(&origin, "!");
	if (*line == ':') line++;
	if (!reverse) {
		format("@%s TAGMSG %s\r\n", tags, line);
	} else if (strstr(tags, "typing=active")) {
		format("PRIVMSG %s :\u2328\uFE0F %s is typing!\r\n", line, nick);
	} else if (strstr(tags, "typing=paused")) {
		format("PRIVMSG %s :\U0001F914 %s is thinking!\r\n", line, nick);
	} else if (strstr(tags, "typing=done")) {
		format("PRIVMSG %s :\u270B %s stopped typing!\r\n", line, nick);
	}
}

int main(int argc, char *argv[]) {
	const char *host = NULL;
	const char *port = "6697";
	const char *cert = NULL;
	const char *nick = "typer";
	const char *user = "typer";
	bool passive = false;

	for (int opt; 0 < (opt = getopt(argc, argv, "PRc:j:n:p:u:v"));) {
		switch (opt) {
			break; case 'P': passive = true;
			break; case 'R': reverse = true;
			break; case 'c': cert = optarg;
			break; case 'j': join = optarg;
			break; case 'n': nick = optarg;
			break; case 'p': port = optarg;
			break; case 'u': user = optarg;
			break; case 'v': verbose = true;
			break; default:  return EX_USAGE;
		}
	}
	if (optind == argc) errx(EX_USAGE, "host required");
	host = argv[optind];

	client = tls_client();
	if (!client) errx(EX_SOFTWARE, "tls_client");

	struct tls_config *config = tls_config_new();
	if (!config) errx(EX_SOFTWARE, "tls_config_new");

	if (cert) {
		int error = tls_config_set_keypair_file(config, cert, cert);
		if (error) errx(EX_CONFIG, "%s: %s", cert, tls_config_error(config));
	}

	int error = tls_configure(client, config);
	if (error) errx(EX_SOFTWARE, "tls_configure: %s", tls_error(client));
	tls_config_free(config);

	error = tls_connect(client, host, port);
	if (error) errx(EX_UNAVAILABLE, "tls_connect: %s", tls_error(client));

	format(
		"CAP REQ :message-tags%s\r\n"
		"NICK %s\r\n"
		"USER %s 0 * :typer\r\n",
		(passive ? " causal.agency/passive" : ""),
		nick, user
	);

	size_t len = 0;
	char buf[4096];
	for (;;) {
		ssize_t read = tls_read(client, &buf[len], sizeof(buf) - len);
		if (read == TLS_WANT_POLLIN || read == TLS_WANT_POLLOUT) continue;
		if (read < 0) errx(EX_IOERR, "tls_read: %s", tls_error(client));
		if (!read) errx(EX_UNAVAILABLE, "server disconnected");
		len += read;

		char *crlf;
		char *line = buf;
		for (;;) {
			crlf = memmem(line, &buf[len] - line, "\r\n", 2);
			if (!crlf) break;
			crlf[0] = '\0';
			if (verbose) printf("%s\n", line);
			handle(line);
			line = &crlf[2];
		}
		len -= line - buf;
		memmove(buf, line, len);
	}
}
