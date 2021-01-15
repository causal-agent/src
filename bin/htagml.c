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

static size_t escape(bool esc, const char *ptr, size_t len) {
	if (!esc) {
		fwrite(ptr, len, 1, stdout);
		return len;
	}
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

static char *hstrstr(const char *haystack, const char *needle) {
	while (haystack) {
		char *elem = strchr(haystack, '<');
		char *match = strstr(haystack, needle);
		if (!match) return NULL;
		if (!elem || match < elem) return match;
		haystack = strchr(elem, '>');
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	bool pre = false;
	bool pipe = false;
	bool index = false;
	const char *tagsFile = "tags";
	for (int opt; 0 < (opt = getopt(argc, argv, "f:ipx"));) {
		switch (opt) {
			break; case 'f': tagsFile = optarg;
			break; case 'i': pipe = true;
			break; case 'p': pre = true;
			break; case 'x': index = true;
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
	printf(pre ? "<pre>" : index ? "<ul class=\"index\">\n" : "");
	while (0 < getline(&buf, &bufCap, file) && ++num) {
		struct Tag *tag = NULL;
		for (size_t i = 0; i < len; ++i) {
			if (tags[i].num) {
				if (num != tags[i].num) continue;
			} else {
				if (regexec(&tags[i].regex, buf, 0, NULL, 0)) continue;
			}
			tag = &tags[i];
			tag->num = num;
			break;
		}
		if (index) {
			if (!tag) continue;
			printf("<li><a class=\"tag\" href=\"#");
			escape(true, tag->tag, strlen(tag->tag));
			printf("\">");
			escape(true, tag->tag, strlen(tag->tag));
			printf("</a></li>\n");
			continue;
		}
		if (pipe) {
			ssize_t len = getline(&buf, &bufCap, stdin);
			if (len < 0) {
				errx(EX_DATAERR, "missing line %d on standard input", num);
			}
		}
		if (!tag) {
			escape(!pipe, buf, strlen(buf));
			continue;
		}

		size_t mlen = strlen(tag->tag);
		char *match = (pipe ? hstrstr : strstr)(buf, tag->tag);
		if (!match && tag->tag[0] == 'M') {
			mlen = 4;
			match = (pipe ? hstrstr : strstr)(buf, "main");
		}
		if (!match) {
			mlen = strlen(buf) - 1;
			match = buf;
		}
		escape(!pipe, buf, match - buf);
		printf("<a class=\"tag\" id=\"");
		escape(true, tag->tag, strlen(tag->tag));
		printf("\" href=\"#");
		escape(true, tag->tag, strlen(tag->tag));
		printf("\">");
		match += escape(!pipe, match, mlen);
		printf("</a>");
		escape(!pipe, match, strlen(match));
	}
	printf(pre ? "</pre>" : index ? "</ul>\n" : "");
}
