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

%option prefix="c11"
%option noyywrap

%{
#include "hilex.h"
%}

%s MacroLine MacroInclude
%x CharLiteral StringLiteral

ident [_[:alpha:]][_[:alnum:]]*
width "*"|[0-9]+

%%
	static int pop = INITIAL;

[[:blank:]]+ { return Normal; }

^"%"[%{}]? {
	BEGIN(pop = MacroLine);
	return Macro;
}

([-+*/%&|^=!<>]|"<<"|">>")"="? |
[=~.?:]|"["|"]"|"++"|"--"|"&&"|"||"|"->" |
sizeof|(_A|alignof) {
	return Operator;
}

([1-9][0-9]*|"0"[0-7]*|"0x"[[:xdigit:]]+)([ulUL]{0,3}) |
([0-9]+("."[0-9]*)?|[0-9]*"."[0-9]+)([eE][+-]?[0-9]+)?[flFL]? |
"0x"[[:xdigit:]]*("."[[:xdigit:]]*)?([pP][+-]?[0-9]+)[flFL]? {
	return Number;
}

auto|break|case|const|continue|default|do|else|enum|extern|for|goto|if|inline |
register|restrict|return|static|struct|switch|typedef|union|volatile|while |
(_A|a)lignas|_Atomic|_Generic|(_N|n)oreturn|(_S|s)tatic_assert |
(_T|t)hread_local {
	return Keyword;
}

^"#"[[:blank:]]*(include|import) {
	BEGIN(pop = MacroInclude);
	return Macro;
}
^"#"[[:blank:]]*{ident} {
	BEGIN(pop = MacroLine);
	return Macro;
}
<MacroInclude>"<"[^>]+">" {
	return String;
}
<MacroLine,MacroInclude>{
	"\n" {
		BEGIN(pop = INITIAL);
		return Normal;
	}
	"\\\n" { return Macro; }
	{ident} { return Macro; }
}

{ident} { return Ident; }

"//"([^\n]|"\\\n")* |
"/*"([^*]|"*"+[^*/])*"*"+"/" {
	return Comment;
}

[LUu]?"'"/[^\\] {
	BEGIN(CharLiteral);
	yymore();
}
[LUu]?"'" {
	BEGIN(CharLiteral);
	return String;
}
([LU]|u8?)?"\""/[^\\%] {
	BEGIN(StringLiteral);
	yymore();
}
([LU]|u8?)?"\"" {
	BEGIN(StringLiteral);
	return String;
}

<CharLiteral,StringLiteral>{
	"\\\n" |
	"\\"[''""?\\abfnrtv] |
	"\\"([0-7]{1,3}) |
	"\\x"([[:xdigit:]]{2}) |
	"\\u"([[:xdigit:]]{4}) |
	"\\U"([[:xdigit:]]{8}) {
		return Escape;
	}
}
<StringLiteral>{
	"%%" |
	"%"[EO]?[ABCDFGHIMRSTUVWXYZabcdeghjmnprtuwxyz] |
	"%"[ #+-0]*{width}?("."{width})?([Lhjltz]|hh|ll)?[AEFGXacdefginopsux] {
		return Format;
	}
}

<CharLiteral>{
	[^\\'']*"'" {
		BEGIN(pop);
		return String;
	}
	[^\\'']+|. { return String; }
}
<StringLiteral>{
	[^%\\""]*"\"" {
		BEGIN(pop);
		return String;
	}
	[^%\\""]+|. { return String; }
}

<MacroLine,MacroInclude>. {
	return Macro;
}

.|\n { return Normal; }

%{
	(void)yyunput;
	(void)input;
%}

%%

const struct Lexer LexC = { yylex, &yyin, &yytext };
