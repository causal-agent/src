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

#include <err.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

enum Class {
	Normal,
	Keyword,
	Macro,
	Comment,
	String,
	ClassCount,
};

struct Syntax {
	enum Class class;
	const char *pattern;
	const char *pattend;
};

// FIXME: Looks like [[:<:]] [[:>:]] are not supported on GNU.
static const struct Syntax CSyntax[] = {
	{ Keyword, "[[:<:]](enum|struct|typedef|union)[[:>:]]", NULL },
	{ Keyword, "[[:<:]](const|inline|static)[[:>:]]", NULL },
	{ Keyword, "[[:<:]](do|else|for|if|switch|while)[[:>:]]", NULL },
	{ Keyword, "[[:<:]](break|continue|goto|return)[[:>:]]", NULL },
	{ Keyword, "[[:<:]](case|default)[[:>:]]", NULL },
	{ Macro,   "^#.*", NULL },
	{ String,  "<[^[:blank:]=]*>", NULL },
	{ Comment, "//.*", NULL },
	{ Comment, "/\\*", "\\*/" },
	{ String,  "[LUu]?'([^']|\\\\')*'", NULL },
	{ String,  "([LUu]|u8)?\"([^\"]|\\\\\")*\"", NULL },
};

static const struct Language {
	const char *name;
	const char *pattern;
	const struct Syntax *syntax;
	size_t len;
} Languages[] = {
	{ "c", "\\.[ch]$", CSyntax, ARRAY_LEN(CSyntax) },
};

static regex_t compile(const char *pattern, int flags) {
	regex_t regex;
	int error = regcomp(&regex, pattern, REG_EXTENDED | REG_NEWLINE | flags);
	if (!error) return regex;
	char buf[256];
	regerror(error, &regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

static void highlight(struct Language lang, enum Class *hi, const char *str) {
	for (size_t i = 0; i < lang.len; ++i) {
		regex_t regex = compile(lang.syntax[i].pattern, 0);
		regex_t regend = {0};
		if (lang.syntax[i].pattend) {
			regend = compile(lang.syntax[i].pattend, 0);
		}

		regmatch_t match = {0};
		for (size_t offset = 0; str[offset]; offset += match.rm_eo) {
			int error = regexec(
				&regex, &str[offset], 1, &match, offset ? REG_NOTBOL : 0
			);
			if (error == REG_NOMATCH) break;
			if (error) errx(EX_SOFTWARE, "regexec: %d", error);

			if (lang.syntax[i].pattend) {
				regmatch_t end;
				error = regexec(
					&regend, &str[offset + match.rm_eo], 1, &end, REG_NOTBOL
				);
				if (error == REG_NOMATCH) break;
				if (error) errx(EX_SOFTWARE, "regexec: %d", error);
				match.rm_eo += end.rm_eo;
			}

			for (regoff_t j = match.rm_so; j < match.rm_eo; ++j) {
				hi[offset + j] = lang.syntax[i].class;
			}
		}
		regfree(&regex);
		if (lang.syntax[i].pattend) regfree(&regend);
	}
}

typedef void HeaderFn(const char *path);
typedef void OutputFn(enum Class class, const char *str, size_t len);

enum SGR {
	Reset, Bold,
	Black = 30, Red, Green, Yellow, Blue, Magenta, Cyan, White, Default,
};

static const enum SGR Style[ClassCount][2] = {
	[Normal]  = { Reset, Default },
	[Keyword] = { Reset, White },
	[Macro]   = { Reset, Green },
	[Comment] = { Reset, Blue },
	[String]  = { Reset, Cyan },
};

static void ansiOutput(enum Class class, const char *str, size_t len) {
	// Style each line separately, otherwise less -R won't look right.
	while (len) {
		size_t line = strcspn(str, "\n") + 1;
		if (line > len) line = len;
		printf(
			"\x1B[%d;%dm%.*s\x1B[%dm",
			Style[class][0], Style[class][1], (int)line, str, Style[class][0]
		);
		str += line;
		len -= line;
	}
}

static void htmlHeader(const char *path) {
	(void)path;
	printf("<pre>");
}
static void htmlFooter(const char *path) {
	(void)path;
	printf("</pre>\n");
}

static void htmlEscape(const char *str, size_t len) {
	while (len) {
		size_t run = strcspn(str, "&<>");
		if (run > len) run = len;
		switch (str[0]) {
			break; case '&': run = 1; printf("&amp;");
			break; case '<': run = 1; printf("&lt;");
			break; case '>': run = 1; printf("&gt;");
			break; default:  printf("%.*s", (int)run, str);
		}
		str += run;
		len -= run;
	}
}

static const char *ClassName[ClassCount] = {
	[Normal]  = "Normal",
	[Keyword] = "Keyword",
	[Macro]   = "Macro",
	[Comment] = "Comment",
	[String]  = "String",
};

static void htmlOutput(enum Class class, const char *str, size_t len) {
	printf("<span class=\"hi %s\">", ClassName[class]);
	htmlEscape(str, len);
	printf("</span>");
}

static void htmlDocumentHeader(const char *path) {
	const char *slash = strrchr(path, '/');
	if (slash) path = &slash[1];
	printf("<!DOCTYPE html>\n<title>");
	htmlEscape(path, strlen(path));
	printf(
		"</title>\n"
		"<style>\n"
		".hi.Keyword { color: dimgray; }\n"
		".hi.Macro   { color: green; }\n"
		".hi.Comment { color: navy; }\n"
		".hi.String  { color: teal; }\n"
		"</style>\n"
	);
	htmlHeader(path);
}

static const struct Format {
	const char *name;
	OutputFn *output;
	HeaderFn *header;
	HeaderFn *footer;
} Formats[] = {
	{ "ansi", ansiOutput, NULL, NULL },
	{ "html", htmlOutput, htmlHeader, htmlFooter },
	{ "html-document", htmlOutput, htmlDocumentHeader, htmlFooter },
};

int main(int argc, char *argv[]) {
	bool check = false;
	const struct Language *lang = NULL;
	const struct Format *format = NULL;
	
	int opt;
	while (0 < (opt = getopt(argc, argv, "cf:l:"))) {
		switch (opt) {
			break; case 'c': check = true;
			break; case 'f': {
				for (size_t i = 0; i < ARRAY_LEN(Formats); ++i) {
					if (strcmp(optarg, Formats[i].name)) continue;
					format = &Formats[i];
					break;
				}
				if (!format) errx(EX_USAGE, "no such format %s", optarg);
			}
			break; case 'l': {
				for (size_t i = 0; i < ARRAY_LEN(Languages); ++i) {
					if (strcmp(optarg, Languages[i].name)) continue;
					lang = &Languages[i];
					break;
				}
				if (!lang) errx(EX_USAGE, "no such language %s", optarg);
			}
			break; default: return EX_USAGE;
		}
	}

	if (check) {
		for (size_t i = 0; i < ARRAY_LEN(Languages); ++i) {
			regex_t regex = compile(Languages[i].pattern, REG_NOSUB);
			regfree(&regex);
			for (size_t j = 0; j < Languages[i].len; ++j) {
				regex = compile(Languages[i].syntax[j].pattern, REG_NOSUB);
				regfree(&regex);
			}
		}
		return EX_OK;
	}

	const char *path = "(stdin)";
	FILE *file = stdin;
	if (optind < argc) {
		path = argv[optind];
		file = fopen(path, "r");
		if (!file) err(EX_NOINPUT, "%s", path);
	}

	if (!lang) {
		for (size_t i = 0; i < ARRAY_LEN(Languages); ++i) {
			regex_t regex = compile(Languages[i].pattern, REG_NOSUB);
			bool match = !regexec(&regex, path, 0, NULL, 0);
			regfree(&regex);
			if (match) {
				lang = &Languages[i];
				break;
			}
		}
		if (!lang) errx(EX_USAGE, "cannot infer language for %s", path);
	}
	if (!format) format = &Formats[0];

	size_t len = 32 * 1024;
	if (file != stdin) {
		struct stat stat;
		int error = fstat(fileno(file), &stat);
		if (error) err(EX_IOERR, "fstat");
		len = stat.st_size;
	}

	char *str = malloc(len + 1);
	if (!str) err(EX_OSERR, "malloc");

	len = fread(str, 1, len, file);
	if (ferror(file)) err(EX_IOERR, "fread");
	str[len] = '\0';

	enum Class *hi = calloc(len, sizeof(*hi));
	if (!hi) err(EX_OSERR, "calloc");

	highlight(*lang, hi, str);

	if (format->header) format->header(path);
	size_t run = 0;
	for (size_t i = 0; i < len; i += run) {
		for (run = 0; i + run < len; ++run) {
			if (hi[i + run] != hi[i]) break;
		}
		format->output(hi[i], &str[i], run);
	}
	if (format->footer) format->footer(path);
}
