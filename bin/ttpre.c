/* Copyright (C) 2018  June McEnroe <june@causal.agency>
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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static void put(const char *tag, wchar_t ch) {
	if (tag) printf("<%s>", tag);
	switch (ch) {
		break; case L'&': printf("&amp;");
		break; case L'<': printf("&lt;");
		break; case L'>': printf("&gt;");
		break; default:   printf("%lc", ch);
	}
	if (tag) printf("</%s>", tag);
}

static void push(wchar_t ch) {
	static wchar_t q[3];
	if (q[1] == L'\b' && q[0] == L'_') {
		put("i", q[2]);
		q[0] = q[1] = q[2] = 0;
	} else if (q[1] == L'\b' && q[0] == q[2]) {
		put("b", q[2]);
		q[0] = q[1] = q[2] = 0;
	} else if (q[0]) {
		put(NULL, q[0]);
	}
	q[0] = q[1];
	q[1] = q[2];
	q[2] = ch;
}

int main(void) {
	setlocale(LC_CTYPE, "");
	printf("<pre>");
	wchar_t ch;
	while (WEOF != (ch = getwchar())) push(ch);
	push(0); push(0); push(0);
	printf("</pre>\n");
	return EXIT_SUCCESS;
}
