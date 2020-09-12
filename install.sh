#!/bin/sh
set -eu

packages='curl htop neovim sl the_silver_searcher tree'

FreeBSD() {
	sudo pkg install ddate $packages
}

OpenBSD() {
	doas pkg_add $packages
}

Linux() {
	sudo pacman -Sy --needed bc ctags gdb openbssh $packages
}

installMacPorts() {
	xcode-select --install
	xcodebuild -license
	dir=MacPorts-2.6.3
	tar=${dir}.tar.bz2
	curl -O "https://distfiles.macports.org/MacPorts/${tar}"
	tar -x -f $tar
	(cd $dir && ./configure)
	make -C $dir
	sudo make -C $dir install
	rm -fr $tar $dir
}

Darwin() {
	[ -d /opt/local ] || installMacPorts
	sudo /opt/local/bin/port selfupdate
	sudo /opt/local/bin/port -N install git pkgconfig $packages
}

$(uname)
