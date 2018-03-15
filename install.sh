#!/bin/sh
set -e -u

any='gnupg htop nasm neovim sl the_silver_searcher tree'
brew="$any ddate git openssh"
pkg="$any curl ddate sudo zsh"
pacman="$any base-devel ctags gdb openssh zsh"

homebrew='https://raw.githubusercontent.com/Homebrew/install/master/install'
if [ "$(uname)" = 'Darwin' ]; then
    xcode-select --install || true
    [ -f /usr/local/bin/brew ] || ruby -e "$(curl -fsSL "$homebrew")"
    exec brew install $brew
fi

[ -f /usr/local/sbin/pkg ] && exec pkg install $pkg
[ -f /usr/bin/pacman ] && pacman -Sy && exec pacman -S --needed $pacman
