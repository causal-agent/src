#!/bin/sh
set -e -u

if [ $# -eq 1 ]; then
	linkPath=$1
	filePath="$PWD/home/${linkPath#$HOME/}"
	[ ! -f "$filePath" ]
	mkdir -p "$(dirname "$filePath")"
	mv "$linkPath" "$filePath"
fi

find home -type f | while read -r findPath; do
	filePath="$PWD/$findPath"
	linkPath="$HOME/${findPath#home/}"
	mkdir -p "$(dirname "$linkPath")"
	[ \( -f "$linkPath" -a -L "$linkPath" \) -o ! -f "$linkPath" ]
	ln -s -f "$filePath" "$linkPath"
done
