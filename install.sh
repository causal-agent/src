#!/bin/bash

# Create symlinks in ~ for files in the current directory.

set -o errexit -o nounset -o pipefail

error() {
  echo "$1"
  exit 1
}

link() {
  local source_path="$PWD/$1"
  local dest_path="$HOME/$1"

  [ -h "$dest_path" ] && return
  [ -e "$dest_path" ] && error "$dest_path exists"

  mkdir -p "$(dirname $dest_path)"
  ln -s "$source_path" "$dest_path"
  echo "$1"
}

link .bin/manpager
link .bin/pbcopy
link .bin/pbd.c
link .bin/pbpaste
link .bin/rpn.c
link .bin/xx.c
link .config/git/config
link .config/git/ignore
link .config/htop/htoprc
link .config/nvim/autoload/pathogen.vim
link .config/nvim/colors/lame.vim
link .config/nvim/init.vim
link .config/nvim/syntax/nasm.vim
link .gnupg/gpg-agent.conf
link .inputrc
link .psqlrc
link .ssh/config
link .tmux.conf
link .zshrc
