#!/usr/bin/env zsh

# Remove symbolic links in ~ to files that no longer exist.

set -o errexit -o nounset -o pipefail

paths=$(find -L ~ -type l -lname "$PWD/*")

for path in $paths; do
  rm "$path"
  echo "$path"
done
