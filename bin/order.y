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
#include <sysexits.h>

static void yyerror(const char *str) {
	errx(EX_DATAERR, "%s", str);
}

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

%}

%token Var Arr Inc Dec Shl Shr Le Ge Eq Ne And Or

%left ','
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
%right '!' '~' Inc Dec
%left '[' ']' Arr '.'

%%

start:
	expr { printf("%s\n", $1); }
	;

expr:
	Var
	| '(' expr ')' { $$ = $2; }
	| expr '[' expr ']' { $$ = fmt("(%s[%s])", $1, $3); }
	| expr Arr Var { $$ = fmt("(%s->%s)", $1, $3); }
	| expr '.' Var { $$ = fmt("(%s.%s)", $1, $3); }
	| '!' expr { $$ = fmt("(!%s)", $2); }
	| '~' expr { $$ = fmt("(~%s)", $2); }
	| Inc expr { $$ = fmt("(++%s)", $2); }
	| Dec expr { $$ = fmt("(--%s)", $2); }
	| expr Inc { $$ = fmt("(%s++)", $1); }
	| expr Dec { $$ = fmt("(%s--)", $1); }
	| '-' expr { $$ = fmt("(-%s)", $2); }
	| '*' expr { $$ = fmt("(*%s)", $2); }
	| '&' expr { $$ = fmt("(&%s)", $2); }
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
	| expr ',' expr { $$ = fmt("(%s, %s)", $1, $3); }
	;

%%

#define T(a, b) ((int)(a) << 8 | (int)(b))

static const char *input;

static int yylex(void) {
	while (isspace(input[0])) input++;
	if (!input[0]) return EOF;

	int len;
	for (len = 0; isalnum(input[len]) || input[len] == '_'; ++len);
	if (len) {
		yylval = fmt("%.*s", len, input);
		input += len;
		return Var;
	}

	int tok = 0;
	switch (T(input[0], input[1])) {
		break; case T('-', '>'): tok = Arr;
		break; case T('+', '+'): tok = Inc;
		break; case T('-', '-'): tok = Dec;
		break; case T('<', '<'): tok = Shl;
		break; case T('>', '>'): tok = Shr;
		break; case T('<', '='): tok = Le;
		break; case T('>', '='): tok = Ge;
		break; case T('=', '='): tok = Eq;
		break; case T('!', '='): tok = Ne;
		break; case T('&', '&'): tok = And;
		break; case T('|', '|'): tok = Or;
	}
	if (tok) {
		input += 2;
		return tok;
	}

	return *input++;
}

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		input = argv[i];
		yyparse();
	}
}
