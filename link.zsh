#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

if [ $# -eq 1 ]; then
    linkPath="$1"
    filePath="$PWD/home/${linkPath#$HOME/}"
    [ ! -f "$filePath" ]
    mkdir -p $(dirname "$filePath")
    mv "$linkPath" "$filePath"
fi

find home -type f | while read findPath; do
    filePath="$PWD/$findPath"
    linkPath="$HOME/${findPath#home/}"
    [ -L "$linkPath" ] && continue
    mkdir -p $(dirname "$linkPath")
    ln -s "$filePath" "$linkPath"
    echo "$linkPath"
done
