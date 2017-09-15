#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

# Remove broken symbolic links in ~.

find -L ~ -type l -lname "$PWD/*" | while read link; do
    rm "$link"
    echo "$link"
done
