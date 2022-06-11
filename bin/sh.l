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
%option noinput nounput noyywrap

%{
#include <assert.h>
#include <stdbool.h>
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

%s Param Command Arith Backtick Subshell
%x DQuote HereDocDel HereDoc HereDocLit

word [[:alnum:]_.-]+
param [^:=?+%#{}-]+
reserved [!{}]|else|do|elif|for|done|fi|then|until|while|if|case|esac

%%
	static bool first;
	static char *delimiter;

[[:blank:]]+ { return Normal; }

"\\". { return Escape; }

<INITIAL,DQuote,HereDoc,Param,Command,Arith,Subshell>{
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
	"(" {
		BEGIN(push(Subshell));
		return Normal;
	}
}
<Param>"}" |
<Command>")" |
<Arith>"))" |
<Backtick>"`" {
	BEGIN(pop());
	return Subst;
}
<Subshell>")" {
	BEGIN(pop());
	return Normal;
}

"\n" {
	first = true;
	return Normal;
}
[&();|]|"&&"|";;"|"||" {
	first = true;
	return Operator;
}
[0-9]?([<>]"&"?|">|"|">>"|"<>") {
	return Operator;
}

{reserved} {
	if (first) {
		first = false;
		return Keyword;
	}
	return Normal;
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

<INITIAL,Command,Backtick,Arith>"#".* { return Comment; }

{word} {
	first = false;
	return Normal;
}

.|\n { return Normal; }

%%

const struct Lexer LexSh = { yylex, &yyin, &yytext };
