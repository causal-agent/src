#!/bin/sh
set -eu

X=
while getopts 'X' opt; do
	case "$opt" in
		(X) X=1;;
		(?) exit 1;;
	esac
done

packages='curl htop sl the_silver_searcher tree'

FreeBSD() {
	pkg install ddate $packages
}

OpenBSD() {
	pkg_add $packages
	if test $X; then
		pkg_add firefox go-fonts imv scrot sct w3m-- xcursor-dmz xsel
	fi
}

Linux() {
	pacman -Sy --needed bc ctags gdb openssh vi $packages
}

Darwin() {
	port selfupdate
	port -N install git mandoc nvi pkgconfig $packages
	mkdir -p /opt/local/etc/select/man
	printf 'bin/man\nshare/man/man1/man.1\nshare/man/man1/man.1.gz\n' \
		>/opt/local/etc/select/man/base
	printf '/usr/bin/man\n/usr/share/man/man1/man.1\n-\n' \
		>/opt/local/etc/select/man/system
	port select --set man system
}

$(uname)
