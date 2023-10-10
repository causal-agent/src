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
#include <ctype.h>
#include <err.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#include "hilex.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

static const char *Class[] = {
#define X(class) [class] = #class,
	ENUM_CLASS
#undef X
};

static FILE *yyin;
static char *yytext;
static int yylex(void) {
	static size_t cap = 0;
	return (getline(&yytext, &cap, yyin) < 0 ? None : Normal);
}
static const struct Lexer LexText = { yylex, &yyin, &yytext };

static const struct {
	const struct Lexer *lexer;
	const char *name;
	const char *namePatt;
	const char *linePatt;
} Lexers[] = {
	{ &LexC, "c", "[.][chlmy]$", NULL },
	{ &LexMake, "make", "[.](mk|am)$|^Makefile$", NULL },
	{ &LexMdoc, "mdoc", "[.][1-9]$", "^[.]Dd" },
	{ &LexSh, "sh", "[.]sh$|^[.](profile|shrc)$", "^#![ ]?/bin/k?sh" },
	{ &LexText, "text", "[.]txt$", NULL },
};

static const struct Lexer *parseLexer(const char *name) {
	for (size_t i = 0; i < ARRAY_LEN(Lexers); ++i) {
		if (!strcmp(name, Lexers[i].name)) return Lexers[i].lexer;
	}
	errx(EX_USAGE, "unknown lexer %s", name);
}

static void ungets(const char *str, FILE *file) {
	size_t len = strlen(str);
	for (size_t i = len-1; i < len; --i) {
		int ch = ungetc(str[i], file);
		if (ch == EOF) errx(EX_IOERR, "cannot push back string");
	}
}

static const struct Lexer *matchLexer(const char *name, FILE *file) {
	char buf[256];
	regex_t regex;
	for (size_t i = 0; i < ARRAY_LEN(Lexers); ++i) {
		int error = regcomp(
			&regex, Lexers[i].namePatt, REG_EXTENDED | REG_NOSUB
		);
		assert(!error);
		error = regexec(&regex, name, 0, NULL, 0);
		regfree(&regex);
		if (!error) return Lexers[i].lexer;
	}
	char *line = fgets(buf, sizeof(buf), file);
	if (!line) return NULL;
	for (size_t i = 0; i < ARRAY_LEN(Lexers); ++i) {
		if (!Lexers[i].linePatt) continue;
		int error = regcomp(
			&regex, Lexers[i].linePatt, REG_EXTENDED | REG_NOSUB
		);
		assert(!error);
		error = regexec(&regex, line, 0, NULL, 0);
		regfree(&regex);
		if (!error) {
			ungets(line, file);
			return Lexers[i].lexer;
		}
	}
	ungets(line, file);
	return NULL;
}

#define ENUM_OPTION \
	X(Document, "document") \
	X(Inline, "inline") \
	X(Monospace, "monospace") \
	X(Pre, "pre") \
	X(Style, "style") \
	X(Tab, "tab") \
	X(Title, "title")

enum Option {
#define X(option, key) option,
	ENUM_OPTION
#undef X
	OptionCap,
};

typedef void Header(const char *opts[]);
typedef void Output(const char *opts[], enum Class class, const char *text);

static bool pager;
static void ansiHeader(const char *opts[]) {
	(void)opts;
	if (!pager) return;
	const char *shell = getenv("SHELL");
	const char *pager = getenv("PAGER");
	if (!shell) shell = "/bin/sh";
	if (!pager) pager = "less";
	setenv("LESS", "FRX", 0);

	int rw[2];
	int error = pipe(rw);
	if (error) err(EX_OSERR, "pipe");

	pid_t pid = fork();
	if (pid < 0) err(EX_OSERR, "fork");
	if (!pid) {
		dup2(rw[0], STDIN_FILENO);
		close(rw[0]);
		close(rw[1]);
		execl(shell, shell, "-c", pager, NULL);
		err(EX_CONFIG, "%s", shell);
	}
	dup2(rw[1], STDOUT_FILENO);
	close(rw[0]);
	close(rw[1]);
	setlinebuf(stdout);

#ifdef __OpenBSD__
	error = pledge("stdio", NULL);
	if (error) err(EX_OSERR, "pledge");
#endif
}

static void ansiFooter(const char *opts[]) {
	(void)opts;
	if (!pager) return;
	int status;
	fclose(stdout);
	wait(&status);
}

static const char *SGR[ClassCap] = {
	[Keyword] = "37",
	[Macro]   = "32",
	[Comment] = "34",
	[String]  = "36",
	[Format]  = "36;1;96",
	[Subst]   = "33",
};

static void ansiFormat(const char *opts[], enum Class class, const char *text) {
	(void)opts;
	if (!SGR[class]) {
		printf("%s", text);
		return;
	}
	// Set color on each line for piping to less -R:
	for (const char *nl; (nl = strchr(text, '\n')); text = &nl[1]) {
		printf("\33[%sm%.*s\33[m\n", SGR[class], (int)(nl - text), text);
	}
	if (*text) printf("\33[%sm%s\33[m", SGR[class], text);
}

static void
debugFormat(const char *opts[], enum Class class, const char *text) {
	if (class != Normal) {
		printf("%s(", Class[class]);
		ansiFormat(opts, class, text);
		printf(")");
	} else {
		printf("%s", text);
	}
}

static const char *IRC[ClassCap] = {
	[Keyword] = "\00315",
	[Macro]   = "\0033",
	[Comment] = "\0032",
	[String]  = "\00310",
	[Format]  = "\00311",
	[Subst]   = "\0037",
};

static void ircHeader(const char *opts[]) {
	if (opts[Monospace]) printf("\21");
}

static const char *stop(const char *text) {
	return (*text == ',' || isdigit(*text) ? "\2\2" : "");
}

static void ircFormat(const char *opts[], enum Class class, const char *text) {
	for (const char *nl; (nl = strchr(text, '\n')); text = &nl[1]) {
		if (IRC[class]) printf("%s%s", IRC[class], stop(text));
		printf("%.*s\n", (int)(nl - text), text);
		if (opts[Monospace]) printf("\21");
	}
	if (*text) {
		if (IRC[class]) {
			printf("%s%s%s\17", IRC[class], stop(text), text);
			if (opts[Monospace]) printf("\21");
		} else {
			printf("%s", text);
		}
	}
}

static void htmlEscape(const char *text) {
	while (*text) {
		switch (*text) {
			break; case '"': text++; printf("&quot;");
			break; case '&': text++; printf("&amp;");
			break; case '<': text++; printf("&lt;");
		}
		size_t len = strcspn(text, "\"&<");
		if (len) fwrite(text, len, 1, stdout);
		text += len;
	}
}

static const char *Styles[ClassCap] = {
	[Keyword] = "color: dimgray;",
	[Macro]   = "color: green;",
	[Comment] = "color: navy;",
	[String]  = "color: teal;",
	[Format]  = "color: teal; font-weight: bold;",
	[Subst]   = "color: olive;",
};

static void styleTabSize(const char *tab) {
	printf("-moz-tab-size: ");
	htmlEscape(tab);
	printf("; tab-size: ");
	htmlEscape(tab);
	printf(";");
}

static void htmlHeader(const char *opts[]) {
	if (!opts[Document]) goto body;

	printf("<!DOCTYPE html>\n<title>");
	if (opts[Title]) htmlEscape(opts[Title]);
	printf("</title>\n");

	if (opts[Style]) {
		printf("<link rel=\"stylesheet\" href=\"");
		htmlEscape(opts[Style]);
		printf("\">\n");
	} else if (!opts[Inline]) {
		printf("<style>\n");
		if (opts[Tab]) {
			printf("pre.hilex { ");
			styleTabSize(opts[Tab]);
			printf(" }\n");
		}
		for (enum Class class = 0; class < ClassCap; ++class) {
			if (!Styles[class]) continue;
			printf("pre.hilex .%.2s { %s }\n", Class[class], Styles[class]);
		}
		printf("</style>\n");
	}

body:
	if ((opts[Document] || opts[Pre]) && opts[Inline] && opts[Tab]) {
		printf("<pre class=\"hilex\" style=\"");
		styleTabSize(opts[Tab]);
		printf("\">");
	} else if (opts[Document] || opts[Pre]) {
		printf("<pre class=\"hilex\">");
	}
}

static void htmlFooter(const char *opts[]) {
	if (opts[Document] || opts[Pre]) printf("</pre>");
	if (opts[Document]) printf("\n");
}

static void htmlFormat(const char *opts[], enum Class class, const char *text) {
	if (class != Normal) {
		if (opts[Inline]) {
			printf("<span style=\"%s\">", Styles[class] ? Styles[class] : "");
		} else {
			printf("<span class=\"%.2s\">", Class[class]);
		}
		htmlEscape(text);
		printf("</span>");
	} else {
		htmlEscape(text);
	}
}

static const struct Formatter {
	const char *name;
	Header *header;
	Output *format;
	Header *footer;
} Formatters[] = {
	{ "ansi", ansiHeader, ansiFormat, ansiFooter },
	{ "debug", NULL, debugFormat, NULL },
	{ "html", htmlHeader, htmlFormat, htmlFooter },
	{ "irc", ircHeader, ircFormat, NULL },
};

static const struct Formatter *parseFormatter(const char *name) {
	for (size_t i = 0; i < ARRAY_LEN(Formatters); ++i) {
		if (!strcmp(name, Formatters[i].name)) return &Formatters[i];
	}
	errx(EX_USAGE, "unknown formatter %s", name);
}

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
	const struct Formatter *formatter = &Formatters[0];
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
		pager = isatty(STDOUT_FILENO);
	}

#ifdef __OpenBSD__
	int error;
	if (formatter->header == ansiHeader && pager) {
		error = pledge("stdio proc exec", NULL);
	} else {
		error = pledge("stdio", NULL);
	}
	if (error) err(EX_OSERR, "pledge");
#endif

	if (!name) {
		if (NULL != (name = strrchr(path, '/'))) {
			name++;
		} else {
			name = path;
		}
	}
	if (!opts[Title]) opts[Title] = name;
	if (!lexer) lexer = matchLexer(name, file);
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
