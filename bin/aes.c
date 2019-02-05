/* Copyright (C) 2019  June McEnroe <june@causal.agency>
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

typedef unsigned char byte;

static const wchar_t Table[128] = {
	L"\x00\x01\x02\x03\x04\x05\x06\x07"
	L"\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
	L"\x10\x11\x12\x13\x14\x15\x16\x17"
	L"\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
	L"　！＂＃＄％＆＇（）＊＋，－．／"
	L"０１２３４５６７８９：；＜＝＞？"
	L"＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ"
	L"ＰＱＲＳＴＵＶＷＸＹＺ［＼］＾＿"
	L"｀ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ"
	L"ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～\x7F"
};

static void enwiden(const char *ch) {
	for (; *ch; ++ch) {
		if ((byte)*ch < 128) printf("%lc", Table[(byte)*ch]);
		else printf("%c", *ch);
	}
}

int main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");
	for (int i = 1; i < argc; ++i) {
		enwiden(argv[i]);
		if (i < argc - 1) printf("%lc", Table[' ']);
		else printf("\n");
	}
	if (argc > 1) return EXIT_SUCCESS;
	char *line = NULL;
	size_t cap = 0;
	while (0 < getline(&line, &cap, stdin)) {
		enwiden(line);
	}
}
