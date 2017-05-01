#!/usr/bin/env zsh

# Remove symbolic links in ~ to files that no longer exist.

set -o errexit -o nounset -o pipefail

find -L ~ -type l -lname "$PWD/*" | while read link; do
  rm "$link"
  echo "$link"
done
