#!/bin/sh
set -eu

common='gdb git gnupg htop sl the_silver_searcher tree'

homebrew=https://raw.githubusercontent.com/Homebrew/install/master/install
macos() {
    xcode-select --install || true
    [ ! -f /usr/local/bin/brew ] && ruby -e "`curl -fsSL $homebrew`"
    brew install $common
    brew install ddate neovim/neovim/neovim openssh
}

freebsd() {
    pkg install $common
    pkg install curl ddate neovim sudo zsh
}

arch() {
    pacman -Sy
    pacman -S --needed base-devel
    pacman -S --needed $common
    pacman -S --needed neovim openssh zsh
}

[ "$(uname)" = 'Darwin' ] && macos
[ -f /usr/local/sbin/pkg ] && freebsd
[ -f /usr/bin/pacman ] && arch
