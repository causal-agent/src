#!/bin/sh

export LANG=en_US.UTF-8
case "$1" in
	(*.[1-9])
		/usr/local/libexec/about-filter "$@"
		printf '</code></pre></td><td><pre><code>'
		;;
	(*)
		exec /usr/local/libexec/hi -t -n "$1" -f html -o anchor
		;;
esac
