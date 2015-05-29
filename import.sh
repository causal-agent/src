#!/bin/bash

# Import a file from ~ and replace it with a symlink.

set -e

error() {
  echo "$1"
  exit 1
}

[ -z "$1" ] && error "no path"

source_path="$HOME/$1"
dest_path="$PWD/$1"

[ -f "$dest_path" ] && error "$dest_path already exists"
[ -f "$source_path" ] || error "$source_path does not exist"

mv "$source_path" "$dest_path"
ln -s "$dest_path" "$source_path"
