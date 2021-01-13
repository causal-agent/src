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

static const char *Styles[ClassCap] = {
	[Keyword]       = "color: dimgray;",
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

	if (opts[Style]) {
		printf("<link rel=\"stylesheet\" href=\"");
		htmlEscape(opts[Style]);
		printf("\">\n");
	} else if (!opts[Inline]) {
		printf("<style>\n");
		if (opts[Tab]) {
			printf("pre.hilex { ");
			styleTabSize(opts[Tab]);
			printf(" }\n");
		}
		for (enum Class class = 0; class < ClassCap; ++class) {
			if (!Styles[class]) continue;
			printf(".hilex.%s { %s }\n", Class[class], Styles[class]);
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

static void htmlFormat(const char *opts[], enum Class class, const char *text) {
	if (class != Normal) {
		if (opts[Inline]) {
			printf("<span style=\"%s\">", Styles[class] ? Styles[class] : "");
		} else {
			printf("<span class=\"hilex %s\">", Class[class]);
		}
		htmlEscape(text);
		printf("</span>");
	} else {
		htmlEscape(text);
	}
}

const struct Formatter FormatHTML = { htmlHeader, htmlFormat, htmlFooter };
