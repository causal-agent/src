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

#include <curl/curl.h>
#include <err.h>
#include <locale.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>

static regex_t regex(const char *pattern, int flags) {
	regex_t regex;
	int error = regcomp(&regex, pattern, REG_EXTENDED | flags);
	if (!error) return regex;

	char buf[256];
	regerror(error, &regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

static const struct Entity {
	wchar_t ch;
	const char *name;
} Entities[] = {
	{ L'"', "&quot;" },
	{ L'&', "&amp;" },
	{ L'<', "&lt;" },
	{ L'>', "&gt;" },
	{ L'␤', "&#10;" },
};

static wchar_t entity(const char *name) {
	for (size_t i = 0; i < sizeof(Entities) / sizeof(Entities[0]); ++i) {
		struct Entity entity = Entities[i];
		if (strncmp(name, entity.name, strlen(entity.name))) continue;
		return entity.ch;
	}
	if (!strncmp(name, "&#x", 3)) return strtoul(&name[3], NULL, 16);
	if (!strncmp(name, "&#", 2)) return strtoul(&name[2], NULL, 10);
	return 0;
}

static const char EntityPattern[] = {
	"[[:space:]]+|&([[:alpha:]]+|#([[:digit:]]+|x[[:xdigit:]]+));"
};
static regex_t EntityRegex;

static void showTitle(const char *title) {
	regmatch_t match = {0};
	for (; *title; title += match.rm_eo) {
		if (regexec(&EntityRegex, title, 1, &match, 0)) break;
		if (title[match.rm_so] != '&') {
			printf("%.*s ", (int)match.rm_so, title);
			continue;
		}
		wchar_t ch = entity(&title[match.rm_so]);
		if (ch) {
			printf("%.*s%lc", (int)match.rm_so, title, (wint_t)ch);
		} else {
			printf("%.*s", (int)match.rm_eo, title);
		}
	}
	printf("%s\n", title);
}

static CURL *curl;
static struct {
	char buf[8192];
	size_t len;
} body;

// HE COMES
static const char TitlePattern[] = "<title>([^<]*)</title>";
static regex_t TitleRegex;

static size_t handleBody(char *buf, size_t size, size_t nitems, void *user) {
	(void)user;

	size_t len = size * nitems;
	size_t cap = sizeof(body.buf) - body.len - 1;
	size_t new = (len < cap ? len : cap);
	if (!new) return 0;

	memcpy(&body.buf[body.len], buf, new);
	body.len += new;
	body.buf[body.len] = '\0';

	regmatch_t match[2];
	if (regexec(&TitleRegex, body.buf, 2, match, 0)) return len;
	body.buf[match[1].rm_eo] = '\0';
	showTitle(&body.buf[match[1].rm_so]);
	return 0;
}

static CURLcode fetchTitle(const char *url) {
	CURLcode code = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (code) return code;

	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	code = curl_easy_perform(curl);
	if (code) return code;

	char *type;
	code = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type);
	if (code) return code;
	if (!type || strncmp(type, "text/html", 9)) return CURLE_OK;

	body.len = 0;
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	code = curl_easy_perform(curl);
	if (code == CURLE_WRITE_ERROR) return CURLE_OK;
	return code;
}

int main(int argc, char *argv[]) {
	EntityRegex = regex(EntityPattern, 0);
	TitleRegex = regex(TitlePattern, REG_ICASE);

	setlocale(LC_CTYPE, "");
	setlinebuf(stdout);

	bool exclude = false;
	regex_t excludeRegex;

	int opt;
	while (0 < (opt = getopt(argc, argv, "x:"))) {
		switch (opt) {
			break; case 'x': {
				exclude = true;
				excludeRegex = regex(optarg, REG_NOSUB);
			}
			break; default:  return EX_USAGE;
		}
	}

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code) errx(EX_OSERR, "curl_global_init: %s", curl_easy_strerror(code));

	curl = curl_easy_init();
	if (!curl) errx(EX_SOFTWARE, "curl_easy_init");

	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "title/1.0");
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleBody);

	static char error[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

	if (optind < argc) {
		code = fetchTitle(argv[optind]);
		if (!code) return EX_OK;
		errx(EX_DATAERR, "curl_easy_perform: %s", error);
	}

	char *buf = NULL;
	size_t cap = 0;

	regex_t urlRegex = regex("https?://[^[:space:]>\"]+", 0);
	while (0 < getline(&buf, &cap, stdin)) {
		regmatch_t match = {0};
		for (char *ptr = buf; *ptr; ptr += match.rm_eo) {
			if (regexec(&urlRegex, ptr, 1, &match, 0)) break;
			ptr[match.rm_eo] = '\0';
			const char *url = &ptr[match.rm_so];
			if (!exclude || regexec(&excludeRegex, url, 0, NULL, 0)) {
				code = fetchTitle(url);
				if (code) warnx("curl_easy_perform: %s", error);
			}
			ptr[match.rm_eo] = ' ';
		}
	}
	if (ferror(stdin)) err(EX_IOERR, "getline");
}
