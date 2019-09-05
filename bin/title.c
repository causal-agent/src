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
#include <strings.h>
#include <sysexits.h>
#include <wchar.h>

static regex_t regex(const char *pattern) {
	regex_t regex;
	int error = regcomp(&regex, pattern, REG_EXTENDED);
	if (!error) return regex;

	char buf[256];
	regerror(error, &regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

static void showTitle(const char *title) {
	printf("%s\n", title);
}

static CURL *curl;
static bool html;
static struct {
	char buf[8192];
	size_t len;
} body;

static const char ContentType[] = "Content-Type: text/html";

static size_t handleHeader(char *buf, size_t size, size_t nitems, void *user) {
	(void)user;
	size_t len = size * nitems;
	if (sizeof(ContentType) - 1 < len) len = sizeof(ContentType) - 1;
	if (!strncasecmp(buf, ContentType, len)) html = true;
	return size * nitems;
}

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

	html = false;
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	code = curl_easy_perform(curl);
	if (code) return code;
	if (!html) return CURLE_OK;

	body.len = 0;
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	code = curl_easy_perform(curl);
	if (code == CURLE_WRITE_ERROR) return CURLE_OK;
	return code;
}

int main(int argc, char *argv[]) {
	TitleRegex = regex(TitlePattern);

	setlocale(LC_CTYPE, "");
	setlinebuf(stdout);

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code) errx(EX_OSERR, "curl_global_init: %s", curl_easy_strerror(code));

	curl = curl_easy_init();
	if (!curl) errx(EX_SOFTWARE, "curl_easy_init");

	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handleHeader);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleBody);

	if (argc > 1) {
		code = fetchTitle(argv[1]);
		if (!code) return EX_OK;
		errx(EX_DATAERR, "curl_easy_perform: %s", curl_easy_strerror(code));
	}

	char *buf = NULL;
	size_t cap = 0;

	regex_t urlRegex = regex("https?://[^[:space:]>\"]+");
	while (0 < getline(&buf, &cap, stdin)) {
		regmatch_t match = {0};
		for (char *url = buf; *url; url += match.rm_eo) {
			if (regexec(&urlRegex, url, 1, &match, 0)) break;
			url[match.rm_eo] = '\0';
			code = fetchTitle(&url[match.rm_so]);
			if (code) warnx("curl_easy_perform: %s", curl_easy_strerror(code));
			url[match.rm_eo] = ' ';
		}
	}
	if (ferror(stdin)) err(EX_IOERR, "getline");
}
