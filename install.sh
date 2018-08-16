#!/bin/sh
set -e -u

any='gnupg htop mksh neovim sl the_silver_searcher tree'
brew="$any ddate git openssh"
pkg="$any curl ddate sudo"
pacman="$any base-devel ctags gdb openssh"

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
[ -f /usr/bin/pacman ] && pacman -Sy && exec pacman -S --needed $pacman
