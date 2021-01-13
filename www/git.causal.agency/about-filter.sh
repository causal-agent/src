#!/bin/sh

options=fragment,man=%N.%S,includes=../tree/%I

case "$1" in
	(README.[1-9])
		exec /usr/bin/mandoc -T html -O $options
		;;
	(*.[1-9])
		exec /usr/bin/mandoc -T html -O $options,toc
		;;
	(*)
		exec /usr/local/libexec/hilex -l text -f html -o pre
		;;
esac
