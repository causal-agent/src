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
#include <stdlib.h>
#include <string.h>

#include "hilex.h"

static void htmlEscape(const char *text) {
	while (*text) {
		switch (*text) {
			break; case '"': text++; printf("&quot;");
			break; case '&': text++; printf("&amp;");
			break; case '<': text++; printf("&lt;");
		}
		size_t len = strcspn(text, "\"&<");
		if (len) fwrite(text, len, 1, stdout);
		text += len;
	}
}

static const char *Class[ClassCap] = {
#define X(class) [class] = #class,
	ENUM_CLASS
#undef X
};

static const char *Style[ClassCap] = {
	[Keyword]       = "color: dimgray;",
	[IdentifierTag] = "color: inherit;",
	[Macro]         = "color: green;",
	[Comment]       = "color: navy;",
	[String]        = "color: teal;",
	[StringFormat]  = "color: teal; font-weight: bold;",
	[Interpolation] = "color: olive;",
};

static void styleTabSize(const char *tab) {
	printf("-moz-tab-size: ");
	htmlEscape(tab);
	printf("; tab-size: ");
	htmlEscape(tab);
	printf(";");
}

static void htmlHeader(const char *opts[]) {
	if (!opts[Document]) goto body;

	printf("<!DOCTYPE html>\n<title>");
	if (opts[Title]) htmlEscape(opts[Title]);
	printf("</title>\n");

	if (opts[CSS]) {
		printf("<link rel=\"stylesheet\" href=\"");
		htmlEscape(opts[CSS]);
		printf("\">\n");
	} else if (!opts[Inline]) {
		printf("<style>\n");
		if (opts[Tab]) {
			printf("pre.hilex { ");
			styleTabSize(opts[Tab]);
			printf(" }\n");
		}
		for (enum Class class = 0; class < ClassCap; ++class) {
			if (!Style[class]) continue;
			printf(".hilex.%s { %s }\n", Class[class], Style[class]);
		}
		if (opts[Anchor]) {
			printf(
				".hilex.%s:target { color: goldenrod; outline: none; }\n",
				Class[IdentifierTag]
			);
		}
		printf("</style>\n");
	}

body:
	if (opts[Inline] && opts[Tab]) {
		printf("<pre class=\"hilex\" style=\"");
		styleTabSize(opts[Tab]);
		printf("\">");
	} else {
		printf("<pre class=\"hilex\">");
	}
}

static void htmlFooter(const char *opts[]) {
	printf("</pre>");
	if (opts[Document]) printf("\n");
}

static void htmlAnchor(const char *opts[], const char *text) {
	if (opts[Inline]) {
		printf("<a style=\"%s\" id=\"", Style[IdentifierTag]);
	} else {
		printf("<a class=\"hilex %s\" id=\"", Class[IdentifierTag]);
	}
	htmlEscape(text);
	printf("\" href=\"#");
	htmlEscape(text);
	printf("\">");
	htmlEscape(text);
	printf("</a>");
}

static void htmlFormat(const char *opts[], enum Class class, const char *text) {
	if (opts[Anchor] && class == IdentifierTag) {
		htmlAnchor(opts, text);
	} else if (class == Normal) {
		htmlEscape(text);
	} else {
		if (opts[Inline]) {
			printf("<span style=\"%s\">", Style[class] ? Style[class] : "");
		} else {
			printf("<span class=\"hilex %s\">", Class[class]);
		}
		htmlEscape(text);
		printf("</span>");
	}
}

const struct Formatter FormatHTML = { htmlHeader, htmlFormat, htmlFooter };
