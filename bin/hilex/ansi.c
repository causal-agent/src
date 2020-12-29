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

static const char *Color[ClassCap] = {
	[Keyword] = "37",
	[Macro] = "32",
	[Comment] = "34",
	[String] = "36",
	[StringFormat] = "36;1;96",
};

static void format(const char *opts[], enum Class class, const char *text) {
	(void)opts;
	if (!Color[class]) {
		printf("%s", text);
		return;
	}
	// Set color on each line for piping to less -R:
	for (const char *nl; (nl = strchr(text, '\n')); text = &nl[1]) {
		printf("\33[%sm%.*s\33[m\n", Color[class], (int)(nl - text), text);
	}
	if (*text) printf("\33[%sm%s\33[m", Color[class], text);
}

const struct Formatter FormatANSI = { .format = format };
