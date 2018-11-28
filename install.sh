#!/bin/sh
set -e -u

pkgAny='curl git gnupg htop mksh sl the_silver_searcher tree'
pkgDarwin='git'
pkgFreeBSD='ddate neovim sudo'
pkgNetBSD='sudo vim'
pkgLinux='base-devel bc ctags gdb neovim openssh'

pkgsrcTag='20171103'
neovimTag='v0.3.1'

pkgsrcTar="bootstrap-trunk-x86_64-${pkgsrcTag}.tar.gz"
pkgsrcURL="https://pkgsrc.joyent.com/packages/Darwin/bootstrap/${pkgsrcTar}"
neovimTar='nvim-macos.tar.gz'
neovimURL="
https://github.com/neovim/neovim/releases/download/${neovimTag}/${neovimTar}
"

Darwin() {
	xcode-select --install || true
	if [ ! -d /opt/pkg ]; then
		curl -O ${pkgsrcURL}
		sudo tar -zxpf ${pkgsrcTar} -C /
		rm ${pkgsrcTar}
	fi
	sudo pkgin update
	sudo pkgin install ${pkgAny} ${pkgDarwin}
	if [ ! -f /usr/local/bin/nvim ]; then
		curl -L -O ${neovimURL}
		sudo tar -xf ${neovimTar} --strip-components 1 -C /usr/local
		rm ${neovimTar}
	fi
}

PKG_PATH="
ftp://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r)/All
"

NetBSD() {
	if [ ! -f /usr/pkg/bin/pkgin ]; then
		export PKG_PATH
		pkg_add pkgin
		echo "${PKG_PATH}" > /usr/pkg/etc/pkgin/repositories.conf
	fi
	pkgin update
	pkgin install ${pkgAny} ${pkgNetBSD}
}

FreeBSD() {
	pkg install ${pkgAny} ${pkgFreeBSD}
}

Linux() {
	pacman -Sy
	pacman -S --needed ${pkgAny} ${pkgLinux}
}

$(uname)
