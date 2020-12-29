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

#include <stdio.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define ENUM_CLASS \
	X(None) \
	X(Normal) \
	X(Operator) \
	X(Number) \
	X(Keyword) \
	X(Identifier) \
	X(Tag) \
	X(Macro) \
	X(Comment) \
	X(String) \
	X(StringEscape) \
	X(StringFormat)

enum Class {
#define X(class) class,
	ENUM_CLASS
#undef X
	ClassCap,
};

typedef int Lex(void);
struct Lexer {
	Lex *lex;
	FILE **in;
	char **text;
};

extern const struct Lexer LexC;
extern const struct Lexer LexMdoc;
extern const struct Lexer LexText;

typedef void Header(const char *opts[]);
typedef void Format(const char *opts[], enum Class class, const char *text);
struct Formatter {
	Header *header;
	Format *format;
	Header *footer;
};

extern const struct Formatter FormatANSI;
extern const struct Formatter FormatDebug;
