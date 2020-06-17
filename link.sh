#!/bin/sh
set -eu

die() {
	echo "$*"
	exit 1
}

if [ $# -eq 1 ]; then
	link=$1
	file="${PWD}/home/${link#${HOME}/}"
	[ ! -f "$file" ] || die "${file} exists"
	mkdir -p "${file%/*}"
	mv "$link" "$file"
fi

find home -type f | while read -r find; do
	file="${PWD}/${find}"
	link="${HOME}/${find#home/}"
	mkdir -p "${link%/*}"
	[ \( -f "$link" -a -L "$link" \) -o ! -f "$link" ] || die "${link} exists"
	ln -fs "$file" "$link"
done
