#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

find -L ~ -type l -lname "$PWD/*" | while read -r linkPath; do
    rm "$linkPath"
    echo "$linkPath"
done
