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
#include <histedit.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

int yylex(void);
void yyerror(const char *str);

#define YYSTYPE uint64_t

enum { RingLen = 64 };
static struct {
	uint64_t vals[RingLen];
	size_t len;
} ring;

static void push(uint64_t val) {
	ring.vals[ring.len++ % RingLen] = val;
}
static uint64_t get(size_t i) {
	return ring.vals[i % RingLen];
}

%}

%token Int Shl Shr Sar

%left '|'
%left '^'
%left '&'
%left Shl Shr Sar
%left '+' '-'
%left '*' '/' '%'
%right '~'
%left 'K' 'M' 'G' 'T'

%%

input:
	expr { push($1); }
	| input ',' expr { push($3); }
	|
	;

expr:
	Int
	| '_' { $$ = get(ring.len - 1); }
	| '[' Int ']' { $$ = get($2); }
	| '(' expr ')' { $$ = $2; }
	| expr 'K' { $$ = $1 << 10; }
	| expr 'M' { $$ = $1 << 20; }
	| expr 'G' { $$ = $1 << 30; }
	| expr 'T' { $$ = $1 << 40; }
	| '~' expr { $$ = ~$2; }
	| '-' expr { $$ = -$2; }
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
	;

%%

#include "bitlex.c"

int yywrap(void) {
	return 1;
}

void yyerror(const char *str) {
	if (yychar < 128 && isprint(yychar)) {
		warnx("%s at '%c'", str, yychar);
	} else {
		warnx("%s at %d", str, yychar);
	}
}

static char *prompt(EditLine *el) {
	(void)el;
	static char buf[64];
	snprintf(buf, sizeof(buf), "[%zu]: ", ring.len);
	return buf;
}

static void print(size_t i) {
	uint64_t val = get(i);

	int bits = val > UINT32_MAX ? 64
		: val > UINT16_MAX ? 32
		: val > UINT8_MAX ? 16
		: 8;

	char bin[65] = {0};
	for (int i = 0; i < 64; ++i) {
		bin[i] = '0' + (val >> (63 - i) & 1);
	}

	printf(
		"[%zu]: %"PRId64" 0x%0*"PRIX64" 0b%s",
		i, (int64_t)val, bits >> 2, val, &bin[64 - bits]
	);

	if (val) {
		if (!(val & (1ULL << 40) - 1)) {
			printf(" %"PRId64"T", val >> 40);
		} else if (!(val & (1 << 30) - 1)) {
			printf(" %"PRId64"G", val >> 30);
		} else if (!(val & (1 << 20) - 1)) {
			printf(" %"PRId64"M", val >> 20);
		} else if (!(val & (1 << 10) - 1)) {
			printf(" %"PRId64"K", val >> 10);
		}
	}

	if (val < 128 && isprint(val)) {
		printf(" '%c'", (char)val);
	}

	printf("\n");
}

int main(void) {
	HistEvent ev;
	History *hist = history_init();
	if (!hist) err(EX_OSERR, "history_init");
	history(hist, &ev, H_SETSIZE, 64);
	history(hist, &ev, H_SETUNIQUE, 1);

	EditLine *el = el_init("bit", stdin, stdout, stderr);
	if (!el) err(EX_IOERR, "el_init");
	el_set(el, EL_PROMPT, prompt);
	el_set(el, EL_HIST, history, hist);

	for (;;) {
		int len;
		const char *line = el_gets(el, &len);
		if (!len) break;

		HistEvent ev;
		history(hist, &ev, H_ENTER, line);

		size_t i = ring.len;

		YY_BUFFER_STATE state = yy_scan_string(line);
		int error = yyparse();
		yy_delete_buffer(state);
		if (error) continue;

		for (; i < ring.len; ++i) {
			print(i);
		}
		printf("\n");
	}

	el_end(el);
	history_end(hist);
}
