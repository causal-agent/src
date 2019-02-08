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

#include <assert.h>
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
	String,
	Escape,
	Format,
	Interp,
	Comment,
	Todo,
	ClassCount,
};

struct Syntax {
	enum Class class;
	enum Class parent;
	const char *pattern;
	const char *pattend;
	size_t subexp;
};

#define WB "(^|[^[:alnum:]_]|\n)"
#define TODO_PATTERN "FIXME|TODO|XXX"

static const struct Syntax CSyntax[] = {
	{ Keyword, .subexp = 2,
		.pattern = WB"(auto|extern|register|static|(_T|t)hread_local)"WB },
	{ Keyword, .subexp = 2,
		.pattern = WB"(const|inline|restrict|volatile)"WB },
	{ Keyword, .subexp = 2,
		.pattern = WB"((_A|a)lignas|_Atomic|(_N|n)oreturn)"WB },
	{ Keyword, .subexp = 2,
		.pattern = WB"(enum|struct|typedef|union)"WB },
	{ Keyword, .subexp = 2,
		.pattern = WB"(case|default|do|else|for|if|switch|while)"WB },
	{ Keyword, .subexp = 2,
		.pattern = WB"(break|continue|goto|return)"WB },
	{ Macro,
		.pattern = "^#(.|\\\\\n)*" },
	{ String, .subexp = 1,
		.pattern = "^#include (<[^>]*>)" },
	{ String,
		.pattern = "[LUu]?'([^']|\\\\')*'" },
	{ String,
		.pattern = "([LU]|u8?)?\"([^\"]|\\\\\")*\"" },
	{ Escape, .parent = String,
		.pattern = "\\\\([\"'?\\abfnrtv]|[0-7]{1,3}|x[0-9A-Fa-f]+)" },
	{ Escape, .parent = String,
		.pattern = "\\\\(U[0-9A-Fa-f]{8}|u[0-9A-Fa-f]{4})" },
	{ Format, .parent = String, .pattern =
		"%%|%[ #+-0]*"         // flags
		"(\\*|[0-9]+)?"        // field width
		"(\\.(\\*|[0-9]+))?"   // precision
		"([Lhjltz]|hh|ll)?"    // length modifier
		"[AEFGXacdefginopsux]" // format specifier
	},
	{ Comment,
		.pattern = "//.*" },
	{ Comment,
		.pattern = "/\\*", .pattend = "\\*/" },
	{ Comment,
		.pattern = "^#if 0", .pattend = "^#endif" },
	{ Todo, .parent = Comment,
		.pattern = TODO_PATTERN },
};

static const struct Syntax MakeSyntax[] = {
	{ Keyword, .subexp = 2,
		.pattern = WB"(\\.(PHONY|PRECIOUS|SUFFIXES))"WB },
	{ Macro,
		.pattern = "^ *-?include" },
	{ String, .subexp = 1,
		.pattern = "[[:alnum:]._]+[[:blank:]]*[!+:?]?=[[:blank:]]*(.*)" },
	{ Normal,
		.pattern = "^\t.*" },
	{ String,
		.pattern = "'([^']|\\\\')*'" },
	{ String,
		.pattern = "\"([^\"]|\\\\\")*\"" },
	{ Interp,
		.pattern = "\\$[^$]" },
	// These Interp patterns handle one level of nesting with the same
	// delimiter.
	{ Interp,
		.pattern = "\\$\\((" "[^$)]" "|" "\\$\\([^)]*\\)" ")*\\)" },
	{ Interp,
		.pattern = "\\$\\{(" "[^$}]" "|" "\\$\\{[^}]*\\}" ")*\\}" },
	{ Escape,
		.pattern = "\\$\\$" },
	{ Comment,
		.pattern = "#.*" },
	{ Todo, .parent = Comment,
		.pattern = TODO_PATTERN },
};

static const struct Language {
	const char *name;
	const char *pattern;
	const struct Syntax *syntax;
	size_t len;
} Languages[] = {
	{ "c", "\\.[ch]$", CSyntax, ARRAY_LEN(CSyntax) },
	{ "make", "Makefile$|\\.mk$", MakeSyntax, ARRAY_LEN(MakeSyntax) },
};

static regex_t compile(const char *pattern, int flags) {
	regex_t regex;
	int error = regcomp(&regex, pattern, REG_EXTENDED | REG_NEWLINE | flags);
	if (!error) return regex;
	char buf[256];
	regerror(error, &regex, buf, sizeof(buf));
	errx(EX_SOFTWARE, "regcomp: %s: %s", buf, pattern);
}

enum { SubsLen = 8 };
static void highlight(struct Language lang, enum Class *hi, const char *str) {
	for (size_t i = 0; i < lang.len; ++i) {
		struct Syntax syn = lang.syntax[i];
		regex_t regex = compile(syn.pattern, 0);
		regex_t regend = {0};
		if (syn.pattend) regend = compile(syn.pattend, 0);

		assert(syn.subexp < SubsLen);
		assert(syn.subexp <= regex.re_nsub);
		regmatch_t subs[SubsLen] = {{0}};
		for (size_t offset = 0; str[offset]; offset += subs[syn.subexp].rm_eo) {
			int error = regexec(
				&regex, &str[offset], SubsLen, subs, offset ? REG_NOTBOL : 0
			);
			if (error == REG_NOMATCH) break;
			if (error) errx(EX_SOFTWARE, "regexec: %d", error);

			if (lang.syntax[i].pattend) {
				regmatch_t end;
				error = regexec(
					&regend, &str[offset + subs[syn.subexp].rm_eo],
					1, &end, REG_NOTBOL
				);
				if (error == REG_NOMATCH) break;
				if (error) errx(EX_SOFTWARE, "regexec: %d", error);
				subs[syn.subexp].rm_eo += end.rm_eo;
			}

			regmatch_t sub = subs[syn.subexp];
			for (regoff_t j = sub.rm_so; j < sub.rm_eo; ++j) {
				if (syn.parent && hi[offset + j] != syn.parent) continue;
				hi[offset + j] = lang.syntax[i].class;
			}
		}
		regfree(&regex);
		if (lang.syntax[i].pattend) regfree(&regend);
	}
}

static void check(void) {
	for (size_t i = 0; i < ARRAY_LEN(Languages); ++i) {
		regex_t regex = compile(Languages[i].pattern, REG_NOSUB);
		regfree(&regex);
		for (size_t j = 0; j < Languages[i].len; ++j) {
			struct Syntax syn = Languages[i].syntax[j];
			regex = compile(syn.pattern, 0);
			if (syn.subexp >= SubsLen || syn.subexp > regex.re_nsub) {
				errx(
					EX_SOFTWARE, "subexpression %zu out of bounds: %s",
					syn.subexp, syn.pattern
				);
			}
			regfree(&regex);
			if (syn.pattend) {
				regex = compile(syn.pattend, REG_NOSUB);
				regfree(&regex);
			}
		}
	}
}

typedef void HeaderFn(const char *path);
typedef void OutputFn(enum Class class, const char *str, size_t len);

enum SGR {
	ANSIReset,
	ANSIBold,
	ANSIBlack = 30,
	ANSIRed,
	ANSIGreen,
	ANSIYellow,
	ANSIBlue,
	ANSIMagenta,
	ANSICyan,
	ANSIWhite,
	ANSIDefault,
};

static const enum SGR ansiStyle[ClassCount][2] = {
	[Normal]  = { ANSIDefault },
	[Keyword] = { ANSIWhite },
	[Macro]   = { ANSIGreen },
	[String]  = { ANSICyan },
	[Escape]  = { ANSIDefault },
	[Format]  = { ANSICyan, ANSIBold },
	[Interp]  = { ANSIGreen },
	[Comment] = { ANSIBlue },
	[Todo]    = { ANSIBlue, ANSIBold },
};

static void ansiOutput(enum Class class, const char *str, size_t len) {
	// Style each line separately, otherwise less -R won't look right.
	while (len) {
		size_t line = strcspn(str, "\n") + 1;
		if (line > len) line = len;
		printf(
			"\x1B[%d;%dm%.*s\x1B[%dm",
			ansiStyle[class][1], ansiStyle[class][0], (int)line, str, ANSIReset
		);
		str += line;
		len -= line;
	}
}

enum IRC {
	IRCWhite,
	IRCBlack,
	IRCBlue,
	IRCGreen,
	IRCRed,
	IRCBrown,
	IRCMagenta,
	IRCOrange,
	IRCYellow,
	IRCLightGreen,
	IRCCyan,
	IRCLightCyan,
	IRCLightBlue,
	IRCPink,
	IRCGray,
	IRCLightGray,
	IRCDefault = 99,
	IRCBold = 0x02,
	IRCColor = 0x03,
	IRCReset = 0x0F,
};

static const enum IRC ircStyle[ClassCount][2] = {
	[Normal]  = { IRCDefault },
	[Keyword] = { IRCGray },
	[Macro]   = { IRCGreen },
	[String]  = { IRCCyan },
	[Escape]  = { IRCDefault },
	[Format]  = { IRCCyan, IRCBold },
	[Interp]  = { IRCGreen },
	[Comment] = { IRCBlue },
	[Todo]    = { IRCBlue, IRCBold },
};

static void ircOutput(enum Class class, const char *str, size_t len) {
	// Style each line separately, for multiple IRC messages.
	while (len) {
		size_t line = strcspn(str, "\n");
		if (line > len) line = len;
		printf(
			"%c%c%02d,%02d%.*s%c",
			ircStyle[class][1] ? ircStyle[class][1] : IRCReset,
			IRCColor, ircStyle[class][0], IRCDefault,
			(int)line, str,
			IRCReset
		);
		// Print newline after all formatting to prevent excess messages.
		if (line < len) {
			printf("\n");
			line++;
		}
		str += line;
		len -= line;
	}
}

static void htmlHeader(const char *path) {
	(void)path;
	printf("<pre class=\"hi\">");
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
	[String]  = "String",
	[Escape]  = "Escape",
	[Format]  = "Format",
	[Interp]  = "Interp",
	[Comment] = "Comment",
	[Todo]    = "Todo",
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
		".hi.String  { color: teal; }\n"
		".hi.Escape  { color: black; }\n"
		".hi.Format  { color: teal; font-weight: bold }\n"
		".hi.Interp  { color: green; }\n"
		".hi.Comment { color: navy; }\n"
		".hi.Todo    { color: navy; font-weight: bold }\n"
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
	{ "irc", ircOutput, NULL, NULL },
	{ "html", htmlOutput, htmlHeader, htmlFooter },
	{ "html-document", htmlOutput, htmlDocumentHeader, htmlFooter },
};

int main(int argc, char *argv[]) {
	const struct Language *lang = NULL;
	const struct Format *format = NULL;
	
	int opt;
	while (0 < (opt = getopt(argc, argv, "cf:l:"))) {
		switch (opt) {
			break; case 'c': {
				check();
				return EX_OK;
			}
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
