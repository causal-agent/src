#!/bin/sh

case "$1" in
	(*.[1-9])
		/usr/bin/mandoc -T html -O fragment,man=%N.%S,includes=../tree/%I
		;;
	(*)
		exec /usr/local/libexec/hi -l text -f html
		;;
esac
