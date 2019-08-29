#!/bin/sh
set -eu

if [ $# -eq 1 ]; then
	link=$1
	file="${PWD}/home/${link#${HOME}/}"
	[ ! -f "$file" ]
	mkdir -p "${file%/*}"
	mv "$link" "$file"
fi

find home -type f | while read -r find; do
	file="${PWD}/${find}"
	link="${HOME}/${find#home/}"
	mkdir -p "${link%/*}"
	[ \( -f "$link" -a -L "$link" \) -o ! -f "$link" ]
	ln -fs "$file" "$link"
done
