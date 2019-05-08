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

%option noinput
%option nounput

%{

#include <stdint.h>

static uint64_t parseInt(uint64_t base, const char *str) {
	uint64_t num = 0;
	for (const char *ch = str; *ch; ++ch) {
		if (*ch == '_') continue;
		num *= base;
		if (*ch >= '0' && *ch <= '9') num += *ch - '0';
		if (*ch >= 'A' && *ch <= 'F') num += 0xA + *ch - 'A';
		if (*ch >= 'a' && *ch <= 'f') num += 0xA + *ch - 'a';
	}
	return num;
}

%}

%%

[ \t\n]	;

0b[01_]+ {
	yylval = parseInt(2, &yytext[2]);
	return Int;
}
0[0-7_]+ {
	yylval = parseInt(8, &yytext[1]);
	return Int;
}
[0-9][0-9_]* {
	yylval = parseInt(10, yytext);
	return Int;
}
0x[0-9A-Fa-f_]+ {
	yylval = parseInt(16, &yytext[2]);
	return Int;
}
'.' {
	yylval = yytext[1];
	return Int;
}

"<<"	return Shl;
">>"	return Shr;
"->>"	return Sar;

.	return yytext[0];
