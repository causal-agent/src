/* Copyright (C) 2020  June McEnroe <june@causal.agency>
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

#include "hilex.h"

static const struct {
	const struct Lexer *lexer;
	const char *name;
	const char *pattern;
} Lexers[] = {
	{ &LexC, "c", "[.][chlmy]$" },
	{ &LexMdoc, "mdoc", "[.][1-9]$" },
	{ &LexText, "text", "[.]txt$" },
};

static const struct Lexer *parseLexer(const char *name) {
	for (size_t i = 0; i < ARRAY_LEN(Lexers); ++i) {
		if (!strcmp(name, Lexers[i].name)) return Lexers[i].lexer;
	}
	errx(EX_USAGE, "unknown lexer %s", name);
}

static const struct Lexer *matchLexer(const char *name) {
	regex_t regex;
	for (size_t i = 0; i < ARRAY_LEN(Lexers); ++i) {
		int error = regcomp(
			&regex, Lexers[i].pattern, REG_EXTENDED | REG_NOSUB
		);
		assert(!error);
		error = regexec(&regex, name, 0, NULL, 0);
		regfree(&regex);
		if (!error) return Lexers[i].lexer;
	}
	return NULL;
}

static const struct {
	const struct Formatter *formatter;
	const char *name;
} Formatters[] = {
	{ &FormatANSI, "ansi" },
	{ &FormatDebug, "debug" },
	{ &FormatHTML, "html" },
	{ &FormatIRC, "irc" },
};

static const struct Formatter *parseFormatter(const char *name) {
	for (size_t i = 0; i < ARRAY_LEN(Formatters); ++i) {
		if (!strcmp(name, Formatters[i].name)) return Formatters[i].formatter;
	}
	errx(EX_USAGE, "unknown formatter %s", name);
}

static const char *ClassName[] = {
#define X(class) [class] = #class,
	ENUM_CLASS
#undef X
};

static void
debugFormat(const char *opts[], enum Class class, const char *text) {
	printf("%s(\33[3m", ClassName[class]);
	FormatANSI.format(opts, class, text);
	printf("\33[m)");
}

const struct Formatter FormatDebug = { .format = debugFormat };

static char *const OptionKeys[OptionCap + 1] = {
#define X(option, key) [option] = key,
	ENUM_OPTION
#undef X
	NULL,
};

int main(int argc, char *argv[]) {
	bool text = false;
	const char *name = NULL;
	const struct Lexer *lexer = NULL;
	const struct Formatter *formatter = &FormatANSI;
	const char *opts[OptionCap] = {0};

	for (int opt; 0 < (opt = getopt(argc, argv, "f:l:n:o:t"));) {
		switch (opt) {
			break; case 'f': formatter = parseFormatter(optarg);
			break; case 'l': lexer = parseLexer(optarg);
			break; case 'n': name = optarg;
			break; case 'o': {
				while (*optarg) {
					char *val;
					int key = getsubopt(&optarg, OptionKeys, &val);
					if (key < 0) errx(EX_USAGE, "no such option %s", val);
					opts[key] = (val ? val : "");
				}
			}
			break; case 't': text = true;
			break; default:  return EX_USAGE;
		}
	}

	const char *path = "(stdin)";
	FILE *file = stdin;
	if (optind < argc) {
		path = argv[optind];
		file = fopen(path, "r");
		if (!file) err(EX_NOINPUT, "%s", path);
	}

	if (!name) {
		if (NULL != (name = strrchr(path, '/'))) {
			name++;
		} else {
			name = path;
		}
	}
	if (!opts[Title]) opts[Title] = name;
	if (!lexer) lexer = matchLexer(name);
	if (!lexer && text) lexer = &LexText;
	if (!lexer) errx(EX_USAGE, "cannot infer lexer for %s", name);

	*lexer->in = file;
	if (formatter->header) formatter->header(opts);
	for (enum Class class; None != (class = lexer->lex());) {
		assert(class < ClassCap);
		formatter->format(opts, class, *lexer->text);
	}
	if (formatter->footer) formatter->footer(opts);
}
