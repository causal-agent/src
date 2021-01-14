#!/bin/sh
set -eu

temp=$(mktemp -d)
trap 'rm -r "${temp}"' EXIT

exec 3>>"${temp}/run.c"

cat >&3 <<EOF
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <dirent.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
EOF

expr=
type=
while getopts 'e:i:t' opt; do
	case "${opt}" in
		(e) expr=$OPTARG;;
		(i) echo "#include <${OPTARG}>" >&3;;
		(t) type=1;;
		(?) exit 1;;
	esac
done
shift $((OPTIND - 1))

cat >&3 <<EOF
int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	$*;
EOF

if [ -n "${type}" ]; then
	cat >&3 <<EOF
	printf(
		_Generic(
			${expr},
			char: "(char) ",
			char *: "(char *) ",
			const char *: "(const char *) ",
			wchar_t *: "(wchar_t *) ",
			const wchar_t *: "(const wchar_t *) ",
			signed char: "(signed char) ",
			short: "(short) ",
			int: "(int) ",
			long: "(long) ",
			long long: "(long long) ",
			unsigned char: "(unsigned char) ",
			unsigned short: "(unsigned short) ",
			unsigned int: "(unsigned int) ",
			unsigned long: "(unsigned long) ",
			unsigned long long: "(unsigned long long) ",
			float: "(float) ",
			double: "(double) ",
			long double: "(long double) ",
			default: "(void *) "
		)
	);
EOF
fi

if [ -n "${expr}" ]; then
	cat >&3 <<EOF
	printf(
		_Generic(
			${expr},
			char: "%c\n",
			char *: "%s\n",
			const char *: "%s\n",
			wchar_t *: "%ls\n",
			const wchar_t *: "%ls\n",
			signed char: "%hhd\n",
			short: "%hd\n",
			int: "%d\n",
			long: "%ld\n",
			long long: "%lld\n",
			unsigned char: "%hhu\n",
			unsigned short: "%hu\n",
			unsigned int: "%u\n",
			unsigned long: "%lu\n",
			unsigned long long: "%llu\n",
			float: "%g\n",
			double: "%g\n",
			long double: "%Lg\n",
			default: "%p\n"
		),
		${expr}
	);
EOF
fi

if [ $# -eq 0 -a -z "${expr}" ]; then
	cat >&3
fi

echo '}' >&3

cat >"${temp}/Makefile" <<EOF
CFLAGS += -Wall -Wextra -Wpedantic
EOF

make -s -C "${temp}" run
"${temp}/run"
