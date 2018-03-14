#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

fail() {
    echo "$@"
    exit 1
}

link() {
    local relPath srcPath dstPath
    < home.txt while read relPath; do
        srcPath="$PWD/home/$relPath"
        dstPath="$HOME/$relPath"
        [ -L "$dstPath" ] && continue
        mkdir -p "$(dirname "$dstPath")"
        ln -s "$srcPath" "$dstPath"
        echo "$relPath"
    done
}

import() {
    local relPath srcPath dstPath
    relPath="$1"
    srcPath="$HOME/$relPath"
    dstPath="$PWD/home/$relPath"
    [ -f "$dstPath" ] && fail "$dstPath exists"
    mkdir -p "$(dirname "$dstPath")"
    mv "$srcPath" "$dstPath"
    ln -s "$dstPath" "$srcPath"
    echo "$relPath" >> home.txt
    sort -o home.txt home.txt
}

prune() {
    local linkPath
    find -L ~ -type l -lname "$PWD/*" | while read linkPath; do
        rm "$linkPath"
        echo "$linkPath"
    done
}

$@
