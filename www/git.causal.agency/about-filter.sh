#!/bin/sh
set -eu

options=fragment,man=%N.%S,includes=../tree/%I

case "$1" in
	(README.[1-9])
		exec mandoc -T html -O $options
		;;
	(*.[1-9])
		exec mandoc -T html -O $options,toc
		;;
	(*)
		exec hilex -l text -f html -o pre
		;;
esac
