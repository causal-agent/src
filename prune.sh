#!/usr/bin/env zsh

# Remove symbolic links in ~ to files that no longer exist.

set -o errexit -o nounset -o pipefail

links=$(find -L ~ -type l -lname "$PWD/*")

for link in $links; do
  rm "$link"
  echo "$link"
done
