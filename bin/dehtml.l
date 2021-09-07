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

%option noyywrap

%{
enum Token {
	Doctype = 1,
	Comment,
	TagOpen,
	TagClose,
	Entity,
	Text,
	Space,
};
%}

%%

"<!DOCTYPE "[^>]*">" { return Doctype; }
"<!--"([^-]|-[^-]|--[^>])*"-->" { return Comment; }
"</"[^>]*">" { return TagClose; }
"<"[^>]*">" { return TagOpen; }
"&"[^;]*";" { return Entity; }
[^<&[:space:]]+ { return Text; }
[[:space:]]+ { return Space; }

%%

#include <err.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sysexits.h>
#include <unistd.h>
#include <wchar.h>

static const struct {
	wchar_t ch;
	const char *name;
} Entities[] = {
	{ L'&', "&amp;" },
	{ L'<', "&lt;" },
	{ L'>', "&gt;" },
	{ L'"', "&quot;" },
	{ L' ', "&nbsp;" },
	{ L'\u00A9', "&copy;" },
	{ L'\u00B7', "&middot;" },
	{ L'\u00BB', "&raquo;" },
	{ L'\u200F', "&rlm;" },
	{ L'\u2014', "&mdash;" },
	{ L'\u2191', "&uarr;" },
};

static void entity(void) {
	wchar_t ch = 0;
	if (yytext[1] == '#') {
		if (yytext[2] == 'x') {
			ch = strtoul(&yytext[3], NULL, 16);
		} else {
			ch = strtoul(&yytext[2], NULL, 10);
		}
	} else {
		for (size_t i = 0; i < sizeof(Entities) / sizeof(Entities[0]); ++i) {
			if (strcmp(Entities[i].name, yytext)) continue;
			ch = Entities[i].ch;
			break;
		}
	}
	if (ch) {
		printf("%lc", (wint_t)ch);
	} else {
		warnx("unknown entity %s", yytext);
		printf("%s", yytext);
	}
}

static bool isTag(const char *tag) {
	const char *ptr = &yytext[1];
	if (*ptr == '/') ptr++;
	size_t len = strlen(tag);
	if (strncasecmp(ptr, tag, len)) return false;
	ptr += len;
	return *ptr == ' ' || *ptr == '>';
}

int main(int argc, char *argv[]) {
	(void)input;
	(void)yyunput;
	setlocale(LC_CTYPE, "");

	bool collapse = 0;
	for (int opt; 0 < (opt = getopt(argc, argv, "s"));) {
		switch (opt) {
			break; case 's': collapse = true;
			break; default:  return EX_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (!argc) argc++;
	for (int i = 0; i < argc; ++i) {
		yyin = (argv[i] ? fopen(argv[i], "r") : stdin);
		if (!yyin) err(EX_NOINPUT, "%s", argv[i]);

		bool space = true;
		bool discard = false;
		bool pre = false;
		for (enum Token tok; (tok = yylex());) {
			if (tok == TagOpen || tok == TagClose) {
				if (isTag("title") || isTag("style") || isTag("script")) {
					discard = (tok == TagOpen);
				} else if (isTag("pre")) {
					pre = (tok == TagOpen);
				}
			} else if (discard) {
				continue;
			} else if (tok == Entity) {
				entity();
				space = false;
			} else if (tok == Text) {
				printf("%s", yytext);
				space = false;
			} else if (tok == Space) {
				if (collapse && !pre) {
					if (space) continue;
					printf("%c", yytext[0]);
				} else {
					printf("%s", yytext);
				}
				space = true;
			}
		}
	}
}
