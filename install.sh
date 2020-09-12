#!/bin/sh
set -eu

pkgAny='curl htop sl the_silver_searcher tree'
pkgDarwin="${pkgAny} git neovim pkg-config"
pkgFreeBSD="${pkgAny} ddate neovim"
pkgLinux="${pkgAny} bc ctags gdb neovim openssh"
pkgNetBSD="${pkgAny} vim"
pkgOpenBSD="${pkgAny} neovim"

Darwin() {
	if [ ! -d /opt/local ]; then
		dir=MacPorts-2.6.3
		tar=${dir}.tar.bz2
		curl -O "https://distfiles.macports.org/MacPorts/${tar}"
		tar -x -f $tar
		(cd $dir && ./configure)
		make -C $dir
		sudo make -C $dir install
		rm -fr $tar $dir
	fi
	sudo /opt/local/bin/port selfupdate
	sudo /opt/local/bin/port -N install $pkgDarwin
}

FreeBSD() {
	pkg install $pkgFreeBSD
}

Linux() {
	pacman -Sy --needed $pkgLinux
}

NetBSD() {
	if [ ! -f /usr/pkg/bin/pkgin ]; then
		base="ftp://ftp.NetBSD.org/pub/pkgsrc/packages"
		export PKG_PATH="${base}/$(uname -s)/$(uname -p)/$(uname -r)/All"
		pkg_add pkgin
		echo "$PKG_PATH" > /usr/pkg/etc/pkgin/repositories.conf
	fi
	pkgin update
	pkgin install $pkgNetBSD
}

OpenBSD() {
	pkg_add $pkgOpenBSD
}

$(uname)
