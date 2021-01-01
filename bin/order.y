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

%{

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define YYSTYPE char *

static char *fmt(const char *format, ...) {
	char *str = NULL;
	va_list ap;
	va_start(ap, format);
	vasprintf(&str, format, ap);
	va_end(ap);
	if (!str) err(EX_OSERR, "vasprintf");
	return str;
}

static int yylex(void);
static void yyerror(const char *str);

%}

%token Ident

%left ','
%right '=' MulAss DivAss ModAss AddAss SubAss ShlAss ShrAss AndAss XorAss OrAss
%right '?' ':'
%left Or
%left And
%left '|'
%left '^'
%left '&'
%left Eq Ne
%left '<' Le '>' Ge
%left Shl Shr
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' Inc Dec Sizeof
%left '(' ')' '[' ']' Arr '.'

%%

start:
	expr { printf("%s\n", $1); }
	;

expr:
	Ident
	| '(' expr ')' { $$ = $2; }
	| expr '[' expr ']' { $$ = fmt("(%s[%s])", $1, $3); }
	| expr Arr Ident { $$ = fmt("(%s->%s)", $1, $3); }
	| expr '.' Ident { $$ = fmt("(%s.%s)", $1, $3); }
	| '!' expr { $$ = fmt("(!%s)", $2); }
	| '~' expr { $$ = fmt("(~%s)", $2); }
	| Inc expr { $$ = fmt("(++%s)", $2); }
	| Dec expr { $$ = fmt("(--%s)", $2); }
	| expr Inc { $$ = fmt("(%s++)", $1); }
	| expr Dec { $$ = fmt("(%s--)", $1); }
	| '+' expr %prec '!' { $$ = fmt("(+%s)", $2); }
	| '-' expr %prec '!' { $$ = fmt("(-%s)", $2); }
	| '*' expr %prec '!' { $$ = fmt("(*%s)", $2); }
	| '&' expr %prec '!' { $$ = fmt("(&%s)", $2); }
	| Sizeof expr { $$ = fmt("(sizeof %s)", $2); }
	| expr '*' expr { $$ = fmt("(%s * %s)", $1, $3); }
	| expr '/' expr { $$ = fmt("(%s / %s)", $1, $3); }
	| expr '%' expr { $$ = fmt("(%s %% %s)", $1, $3); }
	| expr '+' expr { $$ = fmt("(%s + %s)", $1, $3); }
	| expr '-' expr { $$ = fmt("(%s - %s)", $1, $3); }
	| expr Shl expr { $$ = fmt("(%s << %s)", $1, $3); }
	| expr Shr expr { $$ = fmt("(%s >> %s)", $1, $3); }
	| expr '<' expr { $$ = fmt("(%s < %s)", $1, $3); }
	| expr Le expr { $$ = fmt("(%s <= %s)", $1, $3); }
	| expr '>' expr { $$ = fmt("(%s > %s)", $1, $3); }
	| expr Ge expr { $$ = fmt("(%s >= %s)", $1, $3); }
	| expr Eq expr { $$ = fmt("(%s == %s)", $1, $3); }
	| expr Ne expr { $$ = fmt("(%s != %s)", $1, $3); }
	| expr '&' expr { $$ = fmt("(%s & %s)", $1, $3); }
	| expr '^' expr { $$ = fmt("(%s ^ %s)", $1, $3); }
	| expr '|' expr { $$ = fmt("(%s | %s)", $1, $3); }
	| expr And expr { $$ = fmt("(%s && %s)", $1, $3); }
	| expr Or expr { $$ = fmt("(%s || %s)", $1, $3); }
	| expr '?' expr ':' expr { $$ = fmt("(%s ? %s : %s)", $1, $3, $5); }
	| expr ass expr %prec '=' { $$ = fmt("(%s %s %s)", $1, $2, $3); }
	| expr ',' expr { $$ = fmt("(%s, %s)", $1, $3); }
	;

ass:
	'=' { $$ = "="; }
	| MulAss { $$ = "*="; }
	| DivAss { $$ = "/="; }
	| ModAss { $$ = "%="; }
	| AddAss { $$ = "+="; }
	| SubAss { $$ = "-="; }
	| ShlAss { $$ = "<<="; }
	| ShrAss { $$ = ">>="; }
	| AndAss { $$ = "&="; }
	| XorAss { $$ = "^="; }
	| OrAss { $$ = "|="; }
	;

%%

#define T(a, b) ((int)(a) << 8 | (int)(b))

static FILE *in;

static int yylex(void) {
	char ch;
	while (isspace(ch = getc(in)));

	if (isalnum(ch)) {
		char ident[64] = { ch, '\0' };
		for (size_t i = 1; i < sizeof(ident) - 1; ++i) {
			ch = getc(in);
			if (!isalnum(ch) && ch != '_') break;
			ident[i] = ch;
		}
		ungetc(ch, in);
		if (!strcmp(ident, "sizeof")) return Sizeof;
		yylval = fmt("%s", ident);
		return Ident;
	}

	char ne = getc(in);
	switch (T(ch, ne)) {
		case T('-', '>'): return Arr;
		case T('+', '+'): return Inc;
		case T('-', '-'): return Dec;
		case T('<', '='): return Le;
		case T('>', '='): return Ge;
		case T('=', '='): return Eq;
		case T('!', '='): return Ne;
		case T('&', '&'): return And;
		case T('|', '|'): return Or;
		case T('*', '='): return MulAss;
		case T('/', '='): return DivAss;
		case T('%', '='): return ModAss;
		case T('+', '='): return AddAss;
		case T('-', '='): return SubAss;
		case T('&', '='): return AndAss;
		case T('^', '='): return XorAss;
		case T('|', '='): return OrAss;
		case T('<', '<'): {
			if ('=' == (ne = getc(in))) return ShlAss;
			ungetc(ne, in);
			return Shl;
		}
		case T('>', '>'): {
			if ('=' == (ne = getc(in))) return ShrAss;
			ungetc(ne, in);
			return Shr;
		}
		default: {
			ungetc(ne, in);
			return ch;
		}
	}
}

static void yyerror(const char *str) {
	errx(EX_DATAERR, "%s", str);
}

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		in = fmemopen(argv[i], strlen(argv[i]), "r");
		if (!in) err(EX_OSERR, "fmemopen");
		yyparse();
		fclose(in);
	}
	if (argc > 1) return EX_OK;
	in = stdin;
	yyparse();
}
