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
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static void escape(FILE *file, const char *str, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		if (str[i] == '\\' || str[i] == '/') {
			putc('\\', file);
		}
		putc(str[i], file);
	}
}

int main(int argc, char *argv[]) {
	int error;
	bool append = false;
	const char *path = "tags";
	for (int opt; 0 < (opt = getopt(argc, argv, "af:"));) {
		switch (opt) {
			break; case 'a': append = true;
			break; case 'f': path = optarg;
			break; default:  return EX_USAGE;
		}
	}

	FILE *tags = fopen(path, (append ? "a" : "w"));
	if (!tags) err(EX_CANTCREAT, "%s", path);

#ifdef __OpenBSD__
	error = pledge("stdio rpath", NULL);
	if (error) err(EX_OSERR, "pledge");
#endif

	regex_t makeFile, makeLine;
	regex_t mdocFile, mdocLine;
	regex_t shFile, shLine;
	error = 0
		|| regcomp(&makeFile, "(^|/)Makefile|[.]mk$", REG_EXTENDED | REG_NOSUB)
		|| regcomp(
			&makeLine,
			"^([.][^:$A-Z][^:$[:space:]]*|[^.:$][^:$[:space:]]*):",
			REG_EXTENDED
		)
		|| regcomp(&mdocFile, "[.][1-9]$", REG_EXTENDED | REG_NOSUB)
		|| regcomp(&mdocLine, "^[.]S[hs] ([^\t\n]+)", REG_EXTENDED)
		|| regcomp(
			&shFile, "(^|/)[.](profile|shrc)|[.]sh$", REG_EXTENDED | REG_NOSUB
		)
		|| regcomp(&shLine, "^([_[:alnum:]]+)[[:blank:]]*[(][)]", REG_EXTENDED);
	assert(!error);

	size_t cap = 0;
	char *buf = NULL;
	for (int i = optind; i < argc; ++i) {
		const regex_t *regex;
		if (!regexec(&makeFile, argv[i], 0, NULL, 0)) {
			regex = &makeLine;
		} else if (!regexec(&mdocFile, argv[i], 0, NULL, 0)) {
			regex = &mdocLine;
		} else if (!regexec(&shFile, argv[i], 0, NULL, 0)) {
			regex = &shLine;
		} else {
			warnx("skipping unknown file type %s", argv[i]);
			continue;
		}

		FILE *file = fopen(argv[i], "r");
		if (!file) err(EX_NOINPUT, "%s", argv[i]);

		while (0 < getline(&buf, &cap, file)) {
			regmatch_t match[2];
			if (regexec(regex, buf, 2, match, 0)) continue;
			fprintf(
				tags, "%.*s\t%s\t/^",
				(int)(match[1].rm_eo - match[1].rm_so), &buf[match[1].rm_so],
				argv[i]
			);
			escape(tags, buf, match[0].rm_eo);
			fprintf(tags, "/\n");
		}
		fclose(file);
	}
}
