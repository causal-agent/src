#!/bin/bash

# Create symlinks in ~ for files in the current directory.

set -e

error() {
  echo "$1"
  exit 1
}

paths=$(find $PWD -type f -not \( -path '*/.git/*' -o -path '*/Library/*' -o -name '.*.sw?' -o -name 'README.md' -o -name '*.sh' -o -name '*.plist' \))

for source_path in $paths; do
  rel_path="${source_path#$PWD/}"
  dest_path="$HOME/$rel_path"

  [ -h "$dest_path" ] && continue
  [ -e "$dest_path" ] && error "$dest_path exists"

  mkdir -p "$(dirname $dest_path)"
  ln -s "$source_path" "$dest_path"
  echo "$rel_path"
done
