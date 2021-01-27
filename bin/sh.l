/* Copyright (C) 2021  June McEnroe <june@causal.agency>
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

%option prefix="sh"
%option noyywrap

%{
#include <assert.h>
#include <string.h>
#include "hilex.h"

enum { Cap = 64 };
static int len = 1;
static int stack[Cap];
static int push(int val) {
	if (len < Cap) stack[len++] = val;
	return val;
}
static int pop(void) {
	if (len > 1) len--;
	return stack[len-1];
}
%}

%s First
%s Param Command Arith Backtick
%x DQuote HereDocDel HereDoc HereDocLit

word [[:alnum:]_.-]+
param [^:=?+%#{}-]+
reserved [!{}]|else|do|elif|for|done|fi|then|until|while|if|case|esac

%%
	static char *delimiter;

[[:blank:]]+ { return Normal; }

"\\". { return Escape; }

<INITIAL,First,DQuote,HereDoc,Param,Command,Arith>{
	"$"[*@#?$!0-9-] |
	"$"[_[:alpha:][_[:alnum:]]* |
	"${"[#]?{param}"}" {
		return Subst;
	}
	"${"{param} {
		BEGIN(push(Param));
		return Subst;
	}
	"$(" {
		BEGIN(push(Command));
		return Subst;
	}
	"$((" {
		BEGIN(push(Arith));
		return Subst;
	}
	"`" {
		BEGIN(push(Backtick));
		return Subst;
	}
}
<Param>"}" |
<Command>")" |
<Arith>"))" |
<Backtick>"`" {
	BEGIN(pop());
	return Subst;
}

"\n" {
	BEGIN(push(First));
	return Normal;
}
[&();|]|"&&"|";;"|"||" {
	BEGIN(push(First));
	return Operator;
}
[0-9]?([<>]"&"?|">|"|">>"|"<>") {
	return Operator;
}

<First>{
	[[:blank:]]+ { return Normal; }
	{reserved} {
		BEGIN(pop());
		return Keyword;
	}
	{word} {
		BEGIN(pop());
		return Normal;
	}
}

{word}/[[:blank:]]*"()" { return Ident; }

[0-9]?("<<"|"<<-") {
	BEGIN(push(HereDocDel));
	return Operator;
}
<HereDocDel>{
	[[:blank:]]+ { return Normal; }
	{word} {
		delimiter = strdup(yytext);
		assert(delimiter);
		BEGIN(pop(), push(HereDoc));
		return Ident;
	}
	"'"{word}"'" {
		delimiter = strndup(&yytext[1], strlen(yytext)-2);
		assert(delimiter);
		BEGIN(pop(), push(HereDocLit));
		return Ident;
	}
}
<HereDoc,HereDocLit>{
	^"\t"*{word} {
		if (strcmp(&yytext[strspn(yytext, "\t")], delimiter)) REJECT;
		free(delimiter);
		BEGIN(pop());
		return Ident;
	}
}
<HereDoc>{
	[^$`\n]+ { return String; }
	.|\n { return String; }
}
<HereDocLit>{
	.*\n { return String; }
}

"'"[^'']*"'" { return String; }

"\""/[^$`\\] {
	BEGIN(push(DQuote));
	yymore();
}
"\"" {
	BEGIN(push(DQuote));
	return String;
}

<DQuote>{
	[^\\$`""]*"\"" {
		BEGIN(pop());
		return String;
	}
	"\\"[$`""\\\n] { return Escape; }
	[^\\$`""]+|. { return String; }
}

<INITIAL,First,Command,Backtick,Arith>"#".* { return Comment; }

{word} { return Normal; }

.|\n { return Normal; }

%{
	(void)yyunput;
	(void)input;
%}

%%

const struct Lexer LexSh = { yylex, &yyin, &yytext };
