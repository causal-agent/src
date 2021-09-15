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

#include <assert.h>
#include <err.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <tls.h>
#include <unistd.h>

enum { BufferCap = 8192 + 512 };

static bool verbose;
static struct tls *client;

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
	char buf[BufferCap];
	va_list ap;
	va_start(ap, format);
	int len = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	assert((size_t)len < sizeof(buf));
	clientWrite(buf, len);
}

static bool invite;
static const char *join;

enum { Cap = 1024 };
static struct Message {
	char *id;
	char *nick;
	char *chan;
	char *mesg;
} msgs[Cap];
static size_t m;

static void push(struct Message msg) {
	struct Message *dst = &msgs[m++ % Cap];
	free(dst->id);
	free(dst->nick);
	free(dst->chan);
	free(dst->mesg);
	dst->id = strdup(msg.id);
	dst->nick = strdup(msg.nick);
	dst->chan = strdup(msg.chan);
	if (!dst->id || !dst->nick || !dst->chan) err(EX_OSERR, "strdup");
	dst->mesg = NULL;
	if (msg.mesg) {
		dst->mesg = strdup(msg.mesg);
		if (!dst->mesg) err(EX_OSERR, "strdup");
	}
}

static struct Message *find(const char *id) {
	for (size_t i = 0; i < Cap; ++i) {
		if (!msgs[i].id) return NULL;
		if (!strcmp(msgs[i].id, id)) return &msgs[i];
	}
	return NULL;
}

static void handle(char *ptr) {
	char *tags = NULL;
	char *origin = NULL;
	if (ptr && *ptr == '@') tags = 1 + strsep(&ptr, " ");
	if (ptr && *ptr == ':') origin = 1 + strsep(&ptr, " ");
	char *cmd = strsep(&ptr, " ");
	if (!cmd) return;
	if (!strcmp(cmd, "CAP")) {
		strsep(&ptr, " ");
		char *sub = strsep(&ptr, " ");
		if (!sub) errx(EX_PROTOCOL, "CAP without subcommand");
		if (!strcmp(sub, "NAK")) {
			errx(EX_CONFIG, "server does not support %s", ptr);
		} else if (!strcmp(sub, "ACK")) {
			if (!ptr) errx(EX_PROTOCOL, "CAP ACK without caps");
			if (*ptr == ':') ptr++;
			if (!strcmp(ptr, "sasl")) format("AUTHENTICATE EXTERNAL\r\n");
		}
	} else if (!strcmp(cmd, "AUTHENTICATE")) {
		format("AUTHENTICATE +\r\nCAP END\r\n");
	} else if (!strcmp(cmd, "433")) {
		strsep(&ptr, " ");
		char *nick = strsep(&ptr, " ");
		if (!nick) errx(EX_PROTOCOL, "ERR_NICKNAMEINUSE missing nick");
		format("NICK %s_\r\n", nick);
	} else if (!strcmp(cmd, "001")) {
		if (join) format("JOIN %s\r\n", join);
	} else if (!strcmp(cmd, "INVITE") && invite) {
		strsep(&ptr, " ");
		if (!ptr) errx(EX_PROTOCOL, "INVITE missing channel");
		if (*ptr == ':') ptr++;
		format("JOIN %s\r\n", ptr);
	} else if (!strcmp(cmd, "PING")) {
		if (!ptr) errx(EX_PROTOCOL, "PING missing parameter");
		format("PONG %s\r\n", ptr);
	} else if (!strcmp(cmd, "ERROR")) {
		if (!ptr) errx(EX_PROTOCOL, "ERROR missing parameter");
		if (*ptr == ':') ptr++;
		errx(EX_UNAVAILABLE, "%s", ptr);
	}

	if (
		strcmp(cmd, "PRIVMSG") &&
		strcmp(cmd, "NOTICE") &&
		strcmp(cmd, "TAGMSG")
	) return;
	if (!origin) errx(EX_PROTOCOL, "%s missing origin", cmd);

	struct Message msg = {
		.nick = strsep(&origin, "!"),
		.chan = strsep(&ptr, " "),
	};
	if (!msg.chan) errx(EX_PROTOCOL, "%s missing target", cmd);
	if (msg.chan[0] == ':') msg.chan++;
	if (msg.chan[0] != '#') return;
	if (strcmp(cmd, "TAGMSG")) msg.mesg = (*ptr == ':' ? &ptr[1] : ptr);

	if (msg.mesg) {
		if (!strncmp(msg.mesg, "\1ACTION ", 8)) msg.mesg += 8;
		size_t len = strlen(msg.mesg);
		if (msg.mesg[len-1] == '\1') msg.mesg[len-1] = '\0';
	}

	char *reply = NULL;
	char *react = NULL;
	char *typing = NULL;
	if (!tags) return;
	while (tags) {
		char *tag = strsep(&tags, ";");
		char *key = strsep(&tag, "=");
		if (!strcmp(key, "msgid")) {
			if (tag) msg.id = tag;
		} else if (!strcmp(key, "+draft/reply")) {
			if (tag) reply = tag;
		} else if (!strcmp(key, "+draft/react")) {
			if (!tag) continue;
			for (char *ptr = tag; (ptr = strchr(ptr, '\\')); ptr += !!*ptr) {
				switch (ptr[1]) {
					break; case ':': ptr[1] = ';';
					break; case 's': ptr[1] = ' ';
					//break; case 'r': ptr[1] = '\r';
					//break; case 'n': ptr[1] = '\n';
				}
				memmove(ptr, &ptr[1], strlen(&ptr[1]) + 1);
			}
			react = tag;
		} else if (!strcmp(key, "+typing") || !strcmp(key, "+draft/typing")) {
			if (tag) typing = tag;
		}
	}
	if (msg.id) push(msg);

	if (typing) {
		if (!strcmp(typing, "active")) {
			format("NOTICE %s :* %s is typing...\r\n", msg.chan, msg.nick);
		} else if (!strcmp(typing, "paused")) {
			format(
				"NOTICE %s :* %s is thinking hard...\r\n", msg.chan, msg.nick
			);
		} else if (!strcmp(typing, "done")) {
			format("NOTICE %s :* %s has given up :(\r\n", msg.chan, msg.nick);
		} else {
			format(
				"NOTICE %s :* %s is doing some wacky %s typing!\r\n",
				msg.chan, msg.nick, typing
			);
		}
	} else if (react && reply) {
		struct Message *to = find(reply);
		format("NOTICE %s :* %s reacted to ", msg.chan, msg.nick);
		if (to && strcmp(to->chan, msg.chan)) {
			format("a message in another channel");
		} else if (to && to->mesg) {
			size_t len = 0;
			for (size_t n; to->mesg[len]; len += n) {
				n = 1 + strcspn(&to->mesg[len+1], " ");
				if (len + n > 50) break;
			}
			format(
				"%s's message (\"%.*s\"%s)",
				to->nick, (int)len, to->mesg, (to->mesg[len] ? "..." : "")
			);
		} else if (to) {
			format("%s's reaction", to->nick);
		} else {
			format("an unknown message");
		}
		format(" with \"%s\"\r\n", react);
	} else if (react) {
		format(
			"NOTICE %s :* %s reacted to nothing with \"%s\"\r\n",
			msg.chan, msg.nick, react
		);
	} else if (reply) {
		struct Message *to = find(reply);
		format("NOTICE %s :* %s was replying to ", msg.chan, msg.nick);
		if (to && strcmp(to->chan, msg.chan)) {
			format("a message in another channel!\r\n");
		} else if (to && to->mesg) {
			size_t len = 0;
			for (size_t n; to->mesg[len]; len += n) {
				n = 1 + strcspn(&to->mesg[len+1], " ");
				if (len + n > 50) break;
			}
			format(
				"%s's message (\"%.*s\"%s)\r\n",
				to->nick, (int)len, to->mesg, (to->mesg[len] ? "..." : "")
			);
		} else if (to) {
			format("%s's reaction\r\n", to->nick);
		} else {
			format("an unknown message!\r\n");
		}
	}
}

static void quit(int sig) {
	(void)sig;
	format("QUIT\r\n");
	tls_close(client);
	exit(EX_OK);
}

int main(int argc, char *argv[]) {
	const char *host = NULL;
	const char *port = "6697";
	const char *nick = "downgrade";
	const char *cert = NULL;
	const char *priv = NULL;

	for (int opt; 0 < (opt = getopt(argc, argv, "c:ij:k:n:p:v"));) {
		switch (opt) {
			break; case 'c': cert = optarg;
			break; case 'i': invite = true;
			break; case 'j': join = optarg;
			break; case 'k': priv = optarg;
			break; case 'n': nick = optarg;
			break; case 'p': port = optarg;
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
		if (!priv) priv = cert;
		int error = tls_config_set_keypair_file(config, cert, priv);
		if (error) errx(EX_NOINPUT, "%s: %s", cert, tls_config_error(config));
	}

	int error = tls_configure(client, config);
	if (error) errx(EX_SOFTWARE, "tls_configure: %s", tls_error(client));

	error = tls_connect(client, host, port);
	if (error) errx(EX_UNAVAILABLE, "tls_connect: %s", tls_error(client));

	do {
		error = tls_handshake(client);
	} while (error == TLS_WANT_POLLIN || error == TLS_WANT_POLLOUT);
	if (error) errx(EX_PROTOCOL, "tls_handshake: %s", tls_error(client));
	tls_config_clear_keys(config);

	signal(SIGHUP, quit);
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	format(
		"CAP REQ :echo-message message-tags\r\n"
		"NICK %s\r\n"
		"USER %s 0 * :https://causal.agency/bin/downgrade.html\r\n",
		nick, nick
	);
	if (cert) {
		format("CAP REQ sasl\r\n");
	} else {
		format("CAP END\r\n");
	}

	size_t len = 0;
	char buf[BufferCap];
	for (;;) {
		ssize_t n = tls_read(client, &buf[len], sizeof(buf) - len);
		if (n == TLS_WANT_POLLIN || n == TLS_WANT_POLLOUT) continue;
		if (n < 0) errx(EX_IOERR, "tls_read: %s", tls_error(client));
		if (!n) errx(EX_UNAVAILABLE, "disconnected");
		len += n;

		char *ptr = buf;
		for (
			char *crlf;
			(crlf = memmem(ptr, &buf[len] - ptr, "\r\n", 2));
			ptr = crlf + 2
		) {
			*crlf = '\0';
			if (verbose) printf("%s\n", ptr);
			handle(ptr);
		}
		len -= ptr - buf;
		memmove(buf, ptr, len);
	}
}
