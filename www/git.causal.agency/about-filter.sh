#!/bin/sh

export LANG=en_US.UTF-8
case "$1" in
	(*.[1-9])
		/usr/bin/mandoc \
			| /usr/local/libexec/ttpre \
			| /usr/bin/sed -E \
				's,([a-z0-9_-]+)[(]([1-9])[)],<a href="\1.\2">&</a>,g'
		;;
	(*)
		exec /usr/local/libexec/hi -l text -f html
		;;
esac
