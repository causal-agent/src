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
	sudo pacman -Sy --needed bc ctags gdb openssh vi $packages
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
	sudo /opt/local/bin/port -N install git mandoc nvi pkgconfig $packages
	sudo mkdir -p /opt/local/etc/select/man
	printf 'bin/man\nshare/man/man1/man.1\nshare/man/man1/man.1.gz\n' \
		| sudo tee /opt/local/etc/select/man/base >/dev/null
	printf '/usr/bin/man\n/usr/share/man/man1/man.1\n-\n' \
		| sudo tee /opt/local/etc/select/man/system >/dev/null
	sudo port select --set man system
}

$(uname)
