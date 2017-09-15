#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

# Import file from ~ and replace it with a symlink.

fail() {
    echo "$1"
    exit 1
}

[ -z "$1" ] && fail 'no path'

source_path="$HOME/$1"
dest_path="$PWD/home/$1"

[ -f "$dest_path" ] && fail "$dest_path exists"

mkdir -p "$(dirname "$dest_path")"
mv "$source_path" "$dest_path"
ln -s "$dest_path" "$source_path"
echo "link '$1'" >> link.zsh
