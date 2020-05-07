#!/bin/sh
set -eu

pkgAny='curl htop sl the_silver_searcher tree'
pkgDarwin="${pkgAny}"
pkgFreeBSD="${pkgAny} ddate neovim"
pkgNetBSD="${pkgAny} vim"
pkgLinux="${pkgAny} bc ctags gdb neovim openssh"

pkgsrcTag='20171103'
neovimTag='v0.4.3'

Darwin() {
	xcode-select --install || true
	if [ ! -d /opt/pkg ]; then
		tar="bootstrap-trunk-x86_64-${pkgsrcTag}.tar.gz"
		url="https://pkgsrc.joyent.com/packages/Darwin/bootstrap/${tar}"
		curl -O "$url"
		sudo tar -pxz -f "$tar" -C /
		rm "$tar"
	fi
	sudo pkgin update
	sudo pkgin install $pkgDarwin
	sudo ln -fs /opt/pkg/bin/gpg2 /usr/local/bin/gpg
	if [ ! -f /usr/local/bin/nvim ]; then
		tar='nvim-macos.tar.gz'
		base='https://github.com/neovim/neovim/releases/download'
		url="${base}/${neovimTag}/${tar}"
		curl -L -O "$url"
		sudo tar -x -f "$tar" -C /usr/local --strip-components 1
		rm "$tar"
	fi
}

FreeBSD() {
	pkg install $pkgFreeBSD
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
	ln -fs /usr/pkg/bin/gpg2 /usr/local/bin/gpg
}

Linux() {
	pacman -Sy --needed $pkgLinux
}

$(uname)
