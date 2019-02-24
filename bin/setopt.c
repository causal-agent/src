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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

static void printArg(const char *str) {
	printf(" ");
	if (!str[0]) {
		printf("''");
		return;
	}
	while (str[0]) {
		size_t sq = strcspn(str, "'");
		if (sq) {
			printf("'%.*s'", (int)sq, str);
		} else {
			printf("\"'\"");
			sq++;
		}
		str = &str[sq];
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) return EX_USAGE;
	int opt;
	optind = 2;
	while (0 < (optarg = NULL, opt = getopt(argc, argv, argv[1]))) {
		if (opt == '?') return EX_USAGE;
		printf(" -%c", opt);
		if (optarg) printArg(optarg);
	}
	printf(" --");
	for (; optind < argc; ++optind) {
		printArg(argv[optind]);
	}
	printf("\n");
}
