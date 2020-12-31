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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#define MASK(b) ((1ULL << (b)) - 1)

static void yyerror(const char *str);
static int yylex(void);

#define YYSTYPE uint64_t

static uint64_t vars[128];

%}

%left '$'
%right '='
%left '|'
%left '^'
%left '&'
%left Shl Shr Sar
%left '+' '-'
%left '*' '/' '%'
%right '~'
%left 'K' 'M' 'G' 'T'

%token Int Var

%%

stmt:
	expr { vars['_'] = $1; }
	;

expr:
	Int
	| Var { $$ = vars[$1]; }
	| '(' expr ')' { $$ = $2; }
	| expr 'K' { $$ = $1 << 10; }
	| expr 'M' { $$ = $1 << 20; }
	| expr 'G' { $$ = $1 << 30; }
	| expr 'T' { $$ = $1 << 40; }
	| '~' expr { $$ = ~$2; }
	| '&' expr %prec '~' { $$ = MASK($2); }
	| '+' expr %prec '~' { $$ = +$2; }
	| '-' expr %prec '~' { $$ = -$2; }
	| expr '*' expr { $$ = $1 * $3; }
	| expr '/' expr { $$ = $1 / $3; }
	| expr '%' expr { $$ = $1 % $3; }
	| expr '+' expr { $$ = $1 + $3; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr Shl expr { $$ = $1 << $3; }
	| expr Shr expr { $$ = $1 >> $3; }
	| expr Sar expr { $$ = (int64_t)$1 >> $3; }
	| expr '&' expr { $$ = $1 & $3; }
	| expr '^' expr { $$ = $1 ^ $3; }
	| expr '|' expr { $$ = $1 | $3; }
	| Var '=' expr { $$ = vars[$1] = $3; }
	| expr '$' { $$ = $1; }
	;

%%

static void yyerror(const char *str) {
	warnx("%s", str);
}

#define T(a, b) ((int)(a) << 8 | (int)(b))

static const char *input;

static int lexInt(uint64_t base) {
	for (yylval = 0; input[0]; ++input) {
		uint64_t digit;
		if (input[0] == '_') {
			continue;
		} else if (input[0] >= '0' && input[0] <= '9') {
			digit = input[0] - '0';
		} else if (input[0] >= 'A' && input[0] <= 'F') {
			digit = 0xA + input[0] - 'A';
		} else if (input[0] >= 'a' && input[0] <= 'f') {
			digit = 0xA + input[0] - 'a';
		} else {
			return Int;
		}
		if (digit >= base) return Int;
		yylval *= base;
		yylval += digit;
	}
	return Int;
}

static int yylex(void) {
	while (isspace(input[0])) input++;
	if (!input[0]) return EOF;

	if (input[0] == '\'' && input[1] && input[2] == '\'') {
		yylval = input[1];
		input += 3;
		return Int;
	}

	if (input[0] == '0') {
		if (input[1] == 'b') {
			input += 2;
			return lexInt(2);
		} else if (input[1] == 'x') {
			input += 2;
			return lexInt(16);
		} else {
			input += 1;
			return lexInt(8);
		}
	} else if (isdigit(input[0])) {
		return lexInt(10);
	}
	
	if (input[0] == '_' || islower(input[0])) {
		yylval = *input++;
		return Var;
	}

	switch (T(input[0], input[1])) {
		case T('<', '<'): input += 2; return Shl;
		case T('>', '>'): input += 2; return Shr;
		case T('-', '>'): input += 2; return Sar;
		default: return *input++;
	}
}

int main(void) {
	char *line = NULL;
	size_t cap = 0;
	while (0 < getline(&line, &cap, stdin)) {
		if (line[0] == '\n') continue;

		input = line;
		int error = yyparse();
		if (error) continue;

		uint64_t result = vars['_'];

		int bits = result > UINT32_MAX ? 64
			: result > UINT16_MAX ? 32
			: result > UINT8_MAX ? 16
			: 8;

		printf("0x%0*"PRIX64" %"PRId64"", bits >> 2, result, (int64_t)result);

		if (bits == 8) {
			char bin[9] = {0};
			for (int i = 0; i < 8; ++i) {
				bin[i] = '0' + (result >> (7 - i) & 1);
			}
			printf(" 0b%s", bin);
		}

		if (result < 128 && isprint(result)) {
			printf(" '%c'", (char)result);
		}

		if (result) {
			if (!(result & MASK(40))) {
				printf(" %"PRIu64"T", result >> 40);
			} else if (!(result & MASK(30))) {
				printf(" %"PRIu64"G", result >> 30);
			} else if (!(result & MASK(20))) {
				printf(" %"PRIu64"M", result >> 20);
			} else if (!(result & MASK(10))) {
				printf(" %"PRIu64"K", result >> 10);
			}
		}

		printf("\n\n");
	}
}
