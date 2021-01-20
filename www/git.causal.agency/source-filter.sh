#!/bin/sh
set -eu

ctags=/usr/bin/ctags
mtags=/usr/local/libexec/mtags
hilex=/usr/local/libexec/hilex
htagml=/usr/local/libexec/htagml

case "$1" in
	(*.[chlmy]|Makefile|*.mk|*.[1-9])
		tmp=$(mktemp -d -t source-filter)
		trap 'rm -fr "${tmp}"' EXIT
		cd "${tmp}"
		cat >"$1"
		touch tags
		case "$1" in
			(*.[chlmy]) $ctags -w "$1";;
			(*) $mtags "$1";;
		esac
		$hilex -f html "$1" | $htagml -i "$1"
		;;
	(*)
		exec $hilex -t -n "$1" -f html
		;;
esac
