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
	struct Tag {
		char *tag;
		int num;
		regex_t regex;
	} *tags = malloc(cap * sizeof(*tags));
	if (!tags) err(EX_OSERR, "malloc");

	char *buf = NULL;
	size_t bufCap = 0;
	while (0 < getline(&buf, &bufCap, file)) {
		char *line = buf;
		char *tag = strsep(&line, "\t");
		char *file = strsep(&line, "\t");
		char *def = strsep(&line, "\n");
		if (!tag || !file || !def) errx(EX_DATAERR, "malformed tags file");

		if (strcmp(file, name)) continue;
		if (len == cap) {
			tags = realloc(tags, (cap *= 2) * sizeof(*tags));
			if (!tags) err(EX_OSERR, "realloc");
		}
		tags[len].tag = strdup(tag);
		if (!tags[len].tag) err(EX_OSERR, "strdup");

		tags[len].num = 0;
		if (def[0] == '/' || def[0] == '?') {
			def++;
			def[strlen(def)-1] = '\0';
			char *search = nomagic(def);
			int error = regcomp(
				&tags[len].regex, search, REG_NEWLINE | REG_NOSUB
			);
			free(search);
			if (error) {
				warnx("invalid regex for tag %s: %s", tag, def);
				continue;
			}
		} else {
			tags[len].num = strtol(def, &def, 10);
			if (*def) {
				warnx("invalid line number for tag %s: %s", tag, def);
				continue;
			}
		}
		len++;
	}
	fclose(file);

	file = fopen(name, "r");
	if (!file) err(EX_NOINPUT, "%s", name);

	int num = 0;
	if (pre) printf("<pre>");
	while (0 < getline(&buf, &bufCap, file) && ++num) {
		struct Tag *tag = NULL;
		for (size_t i = 0; i < len; ++i) {
			if (tags[i].num) {
				if (num != tags[i].num) continue;
			} else {
				if (regexec(&tags[i].regex, buf, 0, NULL, 0)) continue;
			}
			tag = &tags[i];
			break;
		}
		if (!tag) {
			escape(buf, strlen(buf));
			continue;
		}

		char *text = tag->tag;
		char *match = strstr(buf, text);
		if (!match && tag->tag[0] == 'M') {
			text = "main";
			match = strstr(buf, text);
		}
		if (match) escape(buf, match - buf);
		printf("<a class=\"tag\" id=\"");
		escape(tag->tag, strlen(tag->tag));
		printf("\" href=\"#");
		escape(tag->tag, strlen(tag->tag));
		printf("\">");
		if (match) {
			match += escape(match, strlen(text));
		} else {
			escape(buf, strlen(buf));
		}
		printf("</a>");
		if (match) escape(match, strlen(match));
	}
	if (pre) printf("</pre>");
}
