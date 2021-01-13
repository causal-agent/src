#!/bin/sh
set -eu

ctags=/usr/bin/ctags
hilex=/usr/local/libexec/hilex
htagml=/usr/local/libexec/htagml

case "$1" in
	(*.[chlmy])
		src=$(mktemp -t source-filter)
		tag=$(mktemp -t source-filter)
		trap 'rm -f "${src}" "${tag}"' EXIT
		cat >"${src}"
		$ctags -w -f "${tag}" "${src}"
		$hilex -n "$1" -f html "${src}" | $htagml -i -f "${tag}" "${src}"
		;;
	(*)
		exec $hilex -t -n "$1" -f html
		;;
esac
