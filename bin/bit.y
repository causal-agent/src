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

#define YYSTYPE uint64_t

static int yylex(void);
static void yyerror(const char *str);
static void print(uint64_t val);

static uint64_t vars[128];

%}

%token Int Var

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

%%

stmt:
	| stmt expr '\n' { print(vars['_'] = $2); printf("\n"); }
	| stmt expr ',' { print(vars['_'] = $2); }
	| stmt '\n'
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

static int lexInt(uint64_t base) {
	yylval = 0;
	for (int ch; EOF != (ch = getchar());) {
		uint64_t digit = base;
		if (ch == '_') {
			continue;
		} else if (isdigit(ch)) {
			digit = ch - '0';
		} else if (isxdigit(ch)) {
			digit = 0xA + toupper(ch) - 'A';
		}
		if (digit >= base) {
			ungetc(ch, stdin);
			return Int;
		}
		yylval *= base;
		yylval += digit;
	}
	return Int;
}

static int yylex(void) {
	int ch;
	while (isblank(ch = getchar()));
	if (ch == '\'') {
		yylval = 0;
		while (EOF != (ch = getchar()) && ch != '\'') {
			yylval <<= 8;
			yylval |= ch;
		}
		return Int;
	} else if (ch == '0') {
		ch = getchar();
		if (ch == 'b') {
			return lexInt(2);
		} else if (ch == 'x') {
			return lexInt(16);
		} else {
			ungetc(ch, stdin);
			return lexInt(8);
		}
	} else if (isdigit(ch)) {
		ungetc(ch, stdin);
		return lexInt(10);
	} else if (ch == '_' || islower(ch)) {
		yylval = ch;
		return Var;
	} else if (ch == '<') {
		char ne = getchar();
		if (ne == '<') {
			return Shl;
		} else {
			ungetc(ne, stdin);
			return ch;
		}
	} else if (ch == '-' || ch == '>') {
		char ne = getchar();
		if (ne == '>') {
			return (ch == '-' ? Sar : Shr);
		} else {
			ungetc(ne, stdin);
			return ch;
		}
	} else {
		return ch;
	}
}

static void yyerror(const char *str) {
	warnx("%s", str);
}

static const char *Codes[128] = {
	"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
	"BS",  "HT",  "NL",  "VT",  "NP",  "CR",  "SO",  "SI",
	"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
	"CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
	[127] = "DEL",
};

static void print(uint64_t val) {
	int bits = val > UINT32_MAX ? 64
		: val > UINT16_MAX ? 32
		: val > UINT8_MAX ? 16
		: 8;
	printf("0x%0*"PRIX64" %"PRId64"", bits >> 2, val, (int64_t)val);
	if (bits == 8) {
		char bin[9] = {0};
		for (int i = 0; i < 8; ++i) {
			bin[i] = '0' + (val >> (7 - i) & 1);
		}
		printf(" %#"PRIo64" 0b%s", val, bin);
	}
	if (val < 128) {
		if (isprint(val)) printf(" '%c'", (char)val);
		if (Codes[val]) printf(" %s", Codes[val]);
	}
	if (val) {
		if (!(val & MASK(40))) {
			printf(" %"PRIu64"T", val >> 40);
		} else if (!(val & MASK(30))) {
			printf(" %"PRIu64"G", val >> 30);
		} else if (!(val & MASK(20))) {
			printf(" %"PRIu64"M", val >> 20);
		} else if (!(val & MASK(10))) {
			printf(" %"PRIu64"K", val >> 10);
		}
	}
	printf("\n");
}

int main(void) {
	while (yyparse());
}
