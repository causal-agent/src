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
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sysexits.h>

static regex_t regex(const char *pattern) {
	regex_t regex;
	int error = regcomp(&regex, pattern, REG_EXTENDED);
	if (!error) return regex;
	char buf[256];
	regerror(error, &regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

static CURL *curl;
static bool html;
static struct {
	char buf[8192];
	size_t len;
} body;

static size_t handleHeader(char *buf, size_t size, size_t nitems, void *user) {
	(void)user;
	size_t len = size * nitems;
	const char ContentType[] = "Content-Type: text/html";
	if (sizeof(ContentType) - 1 < len) len = sizeof(ContentType) - 1;
	if (!strncasecmp(buf, ContentType, len)) html = true;
	return size * nitems;
}

static size_t handleBody(char *buf, size_t size, size_t nitems, void *user) {
	(void)user;
	size_t len = size * nitems;
	size_t cap = sizeof(body.buf) - body.len;
	size_t cpy = (len < cap ? len : cap);
	memcpy(&body.buf[body.len], buf, cpy);
	body.len += cpy;
	return len;
}

static const char *TitlePattern = "<title>([^<]*)</title>";
static regex_t TitleRegex;

static bool getTitle(const char *url) {
	CURLcode code = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (code) {
		warnx("CURLOPT_URL: %s", curl_easy_strerror(code));
		return false;
	}

	html = false;
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	if ((code = curl_easy_perform(curl))) {
		warnx("curl_easy_perform: %s", curl_easy_strerror(code));
		return false;
	}
	if (!html) return false;

	body.len = 0;
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	if ((code = curl_easy_perform(curl))) {
		warnx("curl_easy_perform: %s", curl_easy_strerror(code));
		return false;
	}
	body.buf[body.len - 1] = '\0';

	regmatch_t match[2];
	int error = regexec(&TitleRegex, body.buf, 2, match, 0);
	if (error == REG_NOMATCH) return false;
	if (error) errx(EX_SOFTWARE, "regexec: %d", error);

	body.buf[match[1].rm_eo] = '\0';
	char *title = &body.buf[match[1].rm_so];

	printf("%s\n", title);
	return true;
}

int main(int argc, char *argv[]) {
	TitleRegex = regex(TitlePattern);

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code) errx(EX_OSERR, "curl_global_init: %s", curl_easy_strerror(code));

	curl = curl_easy_init();
	if (!curl) errx(EX_SOFTWARE, "curl_easy_init");

	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handleHeader);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleBody);

	setlinebuf(stdout);

	if (argc > 1) {
		return (getTitle(argv[1]) ? EX_OK : EX_DATAERR);
	}

	regex_t urlRegex = regex("https?://[^[:space:]>\"]+");

	char *buf = NULL;
	size_t cap = 0;
	while (0 < getline(&buf, &cap, stdin)) {
		regmatch_t match = {0};
		for (char *url = buf; *url; url += match.rm_eo) {
			int error = regexec(&urlRegex, url, 1, &match, 0);
			if (error == REG_NOMATCH) break;
			if (error) errx(EX_SOFTWARE, "regexec: %d", error);

			url[match.rm_eo] = '\0';
			getTitle(&url[match.rm_so]);
			url[match.rm_eo] = ' ';
		}
	}
}
