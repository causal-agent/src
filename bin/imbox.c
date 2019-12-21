/* Copyright (C) 2019  June McEnroe <june@causal.agency>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <err.h>
#include <readpassphrase.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <tls.h>
#include <unistd.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static void compile(regex_t *regex, const char *pattern) {
	if (regex->re_nsub) return;
	int error = regcomp(regex, pattern, REG_EXTENDED | REG_NEWLINE);
	if (!error) return;
	char buf[256];
	regerror(error, regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

static void mboxrd(const char *headers, const char *body) {
	static regex_t fromRegex;
	compile(&fromRegex, "^From: .*<([^>]+)>");

	regmatch_t from[2];
	int error = regexec(&fromRegex, headers, 2, from, 0);
	if (error) errx(EX_DATAERR, "missing From header");
	printf(
		"From %.*s ",
		(int)(from[1].rm_eo - from[1].rm_so), &headers[from[1].rm_so]
	);

	static regex_t dateRegex;
	compile(&dateRegex, "^Date: (...), (..) (...) (....) (.{8})");

	regmatch_t date[6];
	error = regexec(&dateRegex, headers, 6, date, 0);
	if (error) errx(EX_DATAERR, "missing Date header");
	printf(
		"%.*s %.*s %.*s %.*s %.*s\r\n",
		(int)(date[1].rm_eo - date[1].rm_so), &headers[date[1].rm_so],
		(int)(date[3].rm_eo - date[3].rm_so), &headers[date[3].rm_so],
		(int)(date[2].rm_eo - date[2].rm_so), &headers[date[2].rm_so],
		(int)(date[5].rm_eo - date[5].rm_so), &headers[date[5].rm_so],
		(int)(date[4].rm_eo - date[4].rm_so), &headers[date[4].rm_so]
	);

	printf("%s", headers);
	// FIXME: There seems to sometimes be some garbage data at the end of the
	// headers?

	static regex_t quoteRegex;
	compile(&quoteRegex, "^>*From ");
	regmatch_t match = {0};
	for (const char *ptr = body;; ptr += match.rm_eo) {
		regmatch_t match;
		error = regexec(
			&quoteRegex, body, 1, &match, (ptr > body ? REG_NOTBOL : 0)
		);
		if (error) {
			printf("%s", ptr);
			break;
		}
		printf(
			"%.*s>%.*s",
			(int)match.rm_so, ptr,
			(int)(match.rm_eo - match.rm_so), &ptr[match.rm_so]
		);
	}

	// FIXME: mbox technically shouldn't use \r\n but everything else does...
	printf("\r\n");
}

static bool verbose;

int tlsRead(void *_tls, char *ptr, int len) {
	struct tls *tls = _tls;
	ssize_t ret;
	do {
		ret = tls_read(tls, ptr, len);
	} while (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT);
	if (ret < 0) errx(EX_IOERR, "tls_read: %s", tls_error(tls));
	if (verbose) fprintf(stderr, "%.*s", (int)ret, ptr);
	return ret;
}

int tlsWrite(void *_tls, const char *ptr, int len) {
	struct tls *tls = _tls;
	ssize_t ret;
	do {
		ret = tls_write(tls, ptr, len);
	} while (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT);
	if (ret < 0) errx(EX_IOERR, "tls_write: %s", tls_error(tls));
	if (verbose) fprintf(stderr, "%.*s", (int)ret, ptr);
	return ret;
}

int tlsClose(void *_tls) {
	struct tls *tls = _tls;
	int error = tls_close(tls);
	if (error) errx(EX_IOERR, "tls_close: %s", tls_error(tls));
	return error;
}

#define ENUM_ATOM \
	X(Unknown, "") \
	X(Untagged, "*") \
	X(Ok, "OK") \
	X(No, "NO") \
	X(Bad, "BAD") \
	X(Bye, "BYE") \
	X(Login, "LOGIN") \
	X(Select, "SELECT") \
	X(Search, "SEARCH") \
	X(Fetch, "FETCH")

enum Atom {
#define X(id, _) id,
	ENUM_ATOM
#undef X
};

static const char *Atoms[] = {
#define X(id, str) [id] = str,
	ENUM_ATOM
#undef X
};

static enum Atom atom(const char *str) {
	if (!str) return Unknown;
	for (size_t i = 0; i < ARRAY_LEN(Atoms); ++i) {
		if (!strcmp(str, Atoms[i])) return i;
	}
	return Unknown;
}

static char *readLiteral(FILE *imap, const char *line) {
	char *prefix = strrchr(line, '{');
	if (!prefix) errx(EX_PROTOCOL, "no literal prefix");

	size_t size = strtoul(prefix + 1, NULL, 10);
	if (!size) errx(EX_PROTOCOL, "invalid literal size");

	char *literal = malloc(size);
	if (!literal) err(EX_OSERR, "malloc");

	size_t count = fread(literal, size, 1, imap);
	if (!count) errx(EX_PROTOCOL, "could not read literal");

	return literal;
}

int main(int argc, char *argv[]) {
	const char *host = "imap.fastmail.com";
	const char *port = "993";
	const char *mailbox = "INBOX";
	const char *search = "SUBJECT \"[PATCH\"";
	int rppFlags = 0;

	int opt;
	while (0 < (opt = getopt(argc, argv, "h:m:p:s:vw"))) {
		switch (opt) {
			break; case 'h': host = optarg;
			break; case 'm': mailbox = optarg;
			break; case 'p': port = optarg;
			break; case 's': search = optarg;
			break; case 'v': verbose = true;
			break; case 'w': rppFlags |= RPP_STDIN;
			break; default:  return EX_USAGE;
		}
	}

	const char *user = argv[optind];
	if (!user) errx(EX_USAGE, "username required");

	char buf[1024];
	char *pass = readpassphrase(
		(rppFlags & RPP_STDIN ? "" : "Password: "),
		buf, sizeof(buf), rppFlags
	);
	if (!pass) err(EX_UNAVAILABLE, "readpassphrase");

	struct tls *client = tls_client();
	if (!client) errx(EX_SOFTWARE, "tls_client");

	struct tls_config *config = tls_config_new();
	if (!config) errx(EX_SOFTWARE, "tls_config_new");

	int error = tls_configure(client, config);
	if (error) errx(EX_SOFTWARE, "tls_configure: %s", tls_error(client));
	tls_config_free(config);

	error = tls_connect(client, host, port);
	if (error) errx(EX_NOHOST, "tls_connect: %s", tls_error(client));

	FILE *imap = funopen(client, tlsRead, tlsWrite, NULL, tlsClose);
	if (!imap) err(EX_SOFTWARE, "funopen");

	bool login = false;
	char *uids = NULL;

	char *line = NULL;
	size_t cap = 0;
	while (0 < getline(&line, &cap, imap)) {
		if (strchr(line, '\r')) *strchr(line, '\r') = '\0';

		char *rest = line;
		enum Atom tag = atom(strsep(&rest, " "));
		if (rest && isdigit(rest[0])) strsep(&rest, " ");
		enum Atom resp = atom(strsep(&rest, " "));

		if (resp == No || resp == Bad) {
			errx(
				EX_CONFIG, "%s: %s %s",
				Atoms[tag], Atoms[resp], (rest ? rest : "")
			);
		} else if (resp == Bye) {
			errx(EX_UNAVAILABLE, "unexpected BYE: %s", (rest ? rest : ""));
		}

		switch (tag) {
			break; case Untagged: {
				if (login) break;
				fprintf(imap, "%s LOGIN %s %s\r\n", Atoms[Login], user, pass);
				login = true;
			}
			break; case Login: {
				fprintf(imap, "%s SELECT %s\r\n", Atoms[Select], mailbox);
			}
			break; case Select: {
				fprintf(
					imap, "%s UID SEARCH CHARSET UTF-8 %s\r\n",
					Atoms[Search], search
				);
			}
			break; case Search: {
				if (!uids) errx(EX_PROTOCOL, "no search response");
				for (char *ch = uids; *ch; ++ch) {
					if (*ch == ' ') *ch = ',';
				}
				// FIXME: Grab Content-Encoding as well?
				fprintf(
					imap,
					"%s UID FETCH %s ("
					"BODY[HEADER.FIELDS (Date From Subject Message-Id)] "
					"BODY[TEXT]"
					")\r\n",
					Atoms[Fetch], uids
				);
			}
			break; case Fetch: {
				fprintf(imap, "ayy LOGOUT\r\n");
				fclose(imap);
				return EX_OK;
			}
			break; default:;
		}

		switch (resp) {
			break; case Search: {
				if (!rest) errx(EX_TEMPFAIL, "no messages match");
				uids = strdup(rest);
				if (!uids) err(EX_OSERR, "strdup");
			}
			break; case Fetch: {
				char *headers = readLiteral(imap, rest);

				ssize_t len = getline(&line, &cap, imap);
				if (len <= 0) errx(EX_PROTOCOL, "unexpected eof");

				char *body = readLiteral(imap, line);

				len = getline(&line, &cap, imap);
				if (len <= 0) errx(EX_PROTOCOL, "unexpected eof");
				if (strcmp(line, ")\r\n")) {
					errx(EX_PROTOCOL, "trailing fetch data");
				}

				mboxrd(headers, body);
				free(headers);
				free(body);
			}
			break; default:;
		}
	}
}
