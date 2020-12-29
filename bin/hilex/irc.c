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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hilex.h"

static const char *IRC[ClassCap] = {
	[Keyword]      = "\00315",
	[Macro]        = "\0033",
	[Comment]      = "\0032",
	[String]       = "\00310",
	[StringFormat] = "\00311",
};

static void ircHeader(const char *opts[]) {
	if (opts[Monospace]) printf("\21");
}

static const char *stop(const char *text) {
	return (*text == ',' || isdigit(*text) ? "\2\2" : "");
}

static void ircFormat(const char *opts[], enum Class class, const char *text) {
	for (const char *nl; (nl = strchr(text, '\n')); text = &nl[1]) {
		if (IRC[class]) printf("%s%s", IRC[class], stop(text));
		printf("%.*s\n", (int)(nl - text), text);
		if (opts[Monospace]) printf("\21");
	}
	if (*text) {
		if (IRC[class]) {
			printf("%s%s%s\17", IRC[class], stop(text), text);
			if (opts[Monospace]) printf("\21");
		} else {
			printf("%s", text);
		}
	}
}

const struct Formatter FormatIRC = {
	.header = ircHeader,
	.format = ircFormat,
};
