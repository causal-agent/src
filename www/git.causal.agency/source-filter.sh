#!/bin/sh
set -eu

ctags=/usr/bin/ctags
hilex=/usr/local/libexec/hilex
htagml=/usr/local/libexec/htagml

case "$1" in
	(*.[chlmy])
		tmp=$(mktemp -d -t source-filter)
		trap 'rm -fr "${tmp}"' EXIT
		cd "${tmp}"
		cat >"$1"
		touch tags
		$ctags -w "$1"
		$hilex -f html "$1" | $htagml -i "$1"
		;;
	(*)
		exec $hilex -t -n "$1" -f html
		;;
esac
