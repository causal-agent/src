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
	packages=$(
		echo $packages | sed 's/the_silver_searcher/silversearcher-ag/'
	)
	apt-get install bc build-essential exuberant-ctags gdb nvi $packages
}

Darwin() {
	packages=$(echo $packages | sed 's/the_silver_searcher/ag/')
	cd git/jorts
	git pull
	./Plan git mandoc nvi $packages | sh
}

$(uname)
