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

#include <err.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

struct Tag {
	char *tag;
	regex_t regex;
};

struct Match {
	struct Tag *tag;
	regmatch_t match;
};

static int compar(const void *_a, const void *_b) {
	const struct Match *a = _a;
	const struct Match *b = _b;
	return (a->match.rm_so > b->match.rm_so)
		- (a->match.rm_so < b->match.rm_so);
}

static char *nomagic(const char *pattern) {
	char *buf = malloc(2 * strlen(pattern) + 1);
	if (!buf) err(EX_OSERR, "malloc");
	char *ptr = buf;
	for (const char *ch = pattern; *ch; ++ch) {
		if (strchr(".[*", *ch)) *ptr++ = '\\';
		*ptr++ = *ch;
	}
	*ptr = '\0';
	return buf;
}

static size_t escape(const char *ptr, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		switch (ptr[i]) {
			break; case '&': printf("&amp;");
			break; case '<': printf("&lt;");
			break; case '"': printf("&quot;");
			break; default:  putchar(ptr[i]);
		}
	}
	return len;
}

int main(int argc, char *argv[]) {
	bool pre = false;
	const char *tagsFile = "tags";
	for (int opt; 0 < (opt = getopt(argc, argv, "f:p"));) {
		switch (opt) {
			break; case 'f': tagsFile = optarg;
			break; case 'p': pre = true;
			break; default:  return EX_USAGE;
		}
	}
	if (optind == argc) errx(EX_USAGE, "name required");
	const char *name = argv[optind];

	FILE *file = fopen(tagsFile, "r");
	if (!file) err(EX_NOINPUT, "%s", tagsFile);

	size_t len = 0;
	size_t cap = 256;
	struct Tag *tags = malloc(cap * sizeof(*tags));
	if (!tags) err(EX_OSERR, "malloc");

	char *buf = NULL;
	size_t bufCap = 0;
	while (0 < getline(&buf, &bufCap, file)) {
		char *line = buf;
		char *tag = strsep(&line, "\t");
		char *file = strsep(&line, "\t");
		char *search = strsep(&line, "\n");
		if (!tag || !file || !search) errx(EX_DATAERR, "malformed tags file");
		if (search[0] != '/' || search[strlen(search)-1] != '/') {
			warnx("tag %s definition is not a forward search: %s", tag, search);
			continue;
		}
		search++;
		search[strlen(search)-1] = '\0';

		if (strcmp(file, name)) continue;
		if (len == cap) {
			tags = realloc(tags, (cap *= 2) * sizeof(*tags));
			if (!tags) err(EX_OSERR, "realloc");
		}
		tags[len].tag = strdup(tag);
		if (!tags[len].tag) err(EX_OSERR, "strdup");
		char *pattern = nomagic(search);
		int error = regcomp(&tags[len].regex, pattern, REG_NEWLINE);
		if (error) errx(EX_DATAERR, "invalid regex: %s", pattern);
		free(pattern);
		len++;
	}
	free(buf);
	fclose(file);

	file = fopen(name, "r");
	if (!file) err(EX_NOINPUT, "%s", name);

	struct stat stat;
	int error = fstat(fileno(file), &stat);
	if (error) err(EX_IOERR, "%s", name);
	buf = malloc(stat.st_size + 1);
	if (!buf) err(EX_OSERR, "malloc");

	size_t size = fread(buf, 1, stat.st_size, file);
	if (size < (size_t)stat.st_size && ferror(file)) err(EX_IOERR, "%s", name);
	buf[size] = '\0';
	fclose(file);

	struct Match *matches = calloc(len, sizeof(*matches));
	if (!matches) err(EX_OSERR, "calloc");
	for (size_t i = 0; i < len; ++i) {
		matches[i].tag = &tags[i];
		regexec(&tags[i].regex, buf, 1, &matches[i].match, 0);
	}
	qsort(matches, len, sizeof(*matches), compar);

	char *main;
	const char *base = strrchr(name, '/');
	int n = asprintf(&main, "M%s", (base ? &base[1] : name));
	if (n < 0) err(EX_OSERR, "asprintf");
	if (strrchr(main, '.')) *strrchr(main, '.') = '\0';

	regoff_t pos = 0;
	if (pre) printf("<pre>");
	for (size_t i = 0; i < len; ++i) {
		if (matches[i].match.rm_so == matches[i].match.rm_eo) {
			warnx("no match for tag %s", matches[i].tag->tag);
			continue;
		}
		if (matches[i].match.rm_so <= pos) {
			warnx("overlapping match for tag %s", matches[i].tag->tag);
			continue;
		}

		pos += escape(&buf[pos], matches[i].match.rm_so - pos);
		const char *text = matches[i].tag->tag;
		if (!strcmp(text, main)) text = "main";
		if (!strcmp(text, "yyparse") || !strcmp(text, "yylex")) text = "%%";
		char *tag = strstr(&buf[pos], text);
		if (!tag || tag >= &buf[matches[i].match.rm_eo]) {
			warnx("tag %s does not occur in match", matches[i].tag->tag);
			continue;
		}

		pos += escape(&buf[pos], tag - &buf[pos]);
		printf("<a class=\"tag\" id=\"");
		escape(matches[i].tag->tag, strlen(matches[i].tag->tag));
		printf("\" href=\"#");
		escape(matches[i].tag->tag, strlen(matches[i].tag->tag));
		printf("\">");
		pos += escape(&buf[pos], strlen(text));
		printf("</a>");

		pos += escape(&buf[pos], matches[i].match.rm_eo - pos);
	}
	escape(&buf[pos], strlen(&buf[pos]));
	if (pre) printf("</pre>");
}
