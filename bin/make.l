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

%option prefix="make"
%option noyywrap

%{
#include "hilex.h"
%}

%s Assign Preproc
%x Variable Shell

ident [._[:alnum:]]+
assign [+?:!]?=
target [-._/?*\[\][:alnum:]]+
operator [:!]|::

%%
	static int pop = INITIAL;
	static int depth = 0;

^"\t"+ {
	BEGIN(pop = Shell);
	return Normal;
}
<Shell>{
	"\n" {
		BEGIN(pop = INITIAL);
		return Normal;
	}
	"\\\n" { return Normal; }
	[^\\\n$]+|. { return Normal; }
}

[[:blank:]]+ { return Normal; }

{operator} { return Operator; }

"."(PHONY|PRECIOUS|SUFFIXES)/{operator}? {
	return Keyword;
}

{target}/{operator} { return Ident; }

^"."{ident} |
^"-"?include {
	BEGIN(pop = Preproc);
	return Macro;
}
<Preproc>{
	"\n" {
		BEGIN(pop = INITIAL);
		return Normal;
	}
	"\\\n""\t"? { return Normal; }

	"\""[^""]*"\"" |
	"<"[^>]*">" {
		return String;
	}

	[!<>=]"="?|"||"|"&&" { return Operator; }
	[0-9]+|"0x"[[:xdigit:]]+ { return Number; }
	defined|make|empty|exists|target|commands|in { return Keyword; }
}

^{ident}/[[:blank:]]*{assign} {
	return Ident;
}

{assign} {
	BEGIN(pop = Assign);
	return Operator;
}
<Assign>{
	"\n" {
		BEGIN(pop = INITIAL);
		return Normal;
	}
	"\\\n""\t"? { return Escape; }
	[^\\$[:space:]]+|. { return String; }
}

{target} { return Ident; }

"#"([^\\\n]|"\\"[^\n]|"\\\n")* { return Comment; }

<*>{
	"$"("{"|"(")/[^$] {
		depth++;
		BEGIN(Variable);
		yymore();
	}
	"$"("{"|"(") {
		depth++;
		BEGIN(Variable);
		return Subst;
	}
	"$". { return Subst; }
}
<Variable>{
	[^${}()]*"}"|")" {
		if (!--depth) BEGIN(pop);
		return Subst;
	}
	[^${}()]+ { return Subst; }
}

.|\n { return Normal; }

%{
	(void)yyunput;
	(void)input;
%}

%%

const struct Lexer LexMake = { yylex, &yyin, &yytext };
