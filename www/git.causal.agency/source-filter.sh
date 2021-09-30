#!/bin/sh
set -eu

case "$1" in
	(*.[chlmy]|Makefile|*.mk|*.[1-9]|.profile|.shrc|*.sh)
		tmp=$(mktemp -d)
		trap 'rm -fr "${tmp}"' EXIT
		cd "${tmp}"
		cat >"$1"
		: >tags
		case "$1" in
			(*.[chlmy]) ctags -w "$1";;
			(*) mtags "$1";;
		esac
		hilex -f html "$1" | htagml -i "$1"
		;;
	(*)
		exec hilex -t -n "$1" -f html
		;;
esac
