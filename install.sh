#!/bin/sh
set -eu

common=gdb git gnupg htop the_silver_searcher tmux tree

macos() {
  homebrew=https://raw.githubusercontent.com/Homebrew/install/master/install
  xcode-select --install || true
  [ ! -f /usr/local/bin/brew ] && ruby -e "`curl -fsSL $homebrew`"
  brew install $common
  brew install neovim/neovim/neovim openssh
  brew cask install iterm2
}

freebsd() {
  pkg install $common
  pkg install curl neovim sudo zsh
}

arch() {
  pacman -Sy --needed base-devel
  pacman -Sy --needed $common
  pacman -Sy --needed neovim openssh zsh
}

[ "`uname`" = 'Darwin' ] && exec macos
[ -f /usr/local/sbin/pkg ] && exec freebsd
[ -f /usr/bin/pacman ] && exec arch
