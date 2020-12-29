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

%option prefix="mdoc"
%option noyywrap

%{
#include "hilex.h"
%}

%s MacroLine

%%

^"." {
	BEGIN(MacroLine);
	return Keyword;
}

^".\\\"".* { return Comment; }

<MacroLine>{
	"\n" {
		BEGIN(0);
		return Normal;
	}

	%[ABCDIJNOPQRTUV]|A[cdnopqrt]|B[cdfkloqtx]|Br[coq]|Bsx|C[dm]|D[1bcdloqtvx] |
	E[cdfklmnorsvx]|F[acdlnortx]|Hf|I[cnt]|L[bikp]|M[st]|N[dmosx]|O[copstx] |
	P[acfopq]|Q[cloq]|R[esv]|S[chmoqstxy]|T[an]|U[dx]|V[at]|X[cor] {
		return Keyword;
	}

	"\""([^""]|"\\\"")*"\"" { return String; }
}

"\\"(.|"("..|"["[^]]*"]") { return String; }

[[:blank:]]+|[^.\n""\\[:space:]]+|.|\n { return Normal; }

%{
	(void)yyunput;
	(void)input;
%}

%%

const struct Lexer LexMdoc = { yylex, &yyin, &yytext };
