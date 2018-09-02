#!/bin/sh
set -e -u

any='gnupg htop mksh sl the_silver_searcher tree'
brew="$any ddate git neovim openssh"
pkg="$any curl ddate neovim sudo"
pkgin="$any curl sudo vim"
pacman="$any base-devel bc ctags gdb neovim openssh"

homebrew='https://raw.githubusercontent.com/Homebrew/install/master/install'
if [ "$(uname)" = 'Darwin' ]; then
	xcode-select --install || true
	[ -f /usr/local/bin/brew ] || ruby -e "$(curl -fsSL "$homebrew")"
	brew install $brew || true
	if ! grep -q 'mksh' /etc/shells; then
		echo '/usr/local/bin/mksh' | sudo tee -a /etc/shells > /dev/null
	fi
	exit
fi

[ -f /usr/local/sbin/pkg ] && exec pkg install $pkg

if [ "$(uname)" = 'NetBSD' ]; then
	export PKG_PATH="ftp://ftp.netbsd.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r)/All"
	pkg_add pkgin
	echo "$PKG_PATH" > /usr/pkg/etc/pkgin/repositories.conf
	pkgin update
	pkgin install $pkgin
	exit
fi

[ -f /usr/bin/pacman ] && pacman -Sy && exec pacman -S --needed $pacman
