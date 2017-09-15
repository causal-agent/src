#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

# Create symbolic links in ~.

fail() {
    echo "$1"
    exit 1
}

link() {
    local source_path="$PWD/home/$1"
    local dest_path="$HOME/$1"

    [ -L "$dest_path" ] && return

    mkdir -p "$(dirname "$dest_path")"
    ln -s "$source_path" "$dest_path"
    echo "$1"
}

link '.bin/sup'
link '.bin/tup'
link '.bin/up'
link '.config/git/config'
link '.config/git/ignore'
link '.config/htop/htoprc'
link '.config/nvim/colors/trivial.vim'
link '.config/nvim/init.vim'
link '.config/nvim/syntax/nasm.vim'
link '.gdbinit'
link '.gnupg/gpg-agent.conf'
link '.hushlogin'
link '.inputrc'
link '.psqlrc'
link '.ssh/config'
link '.zshrc'
