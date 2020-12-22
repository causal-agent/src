#!/bin/sh
set -eu

find -L ~/.config ~/.local -type l -lname "${PWD}/*" | while read -r link; do
	echo "$link"
	rm "$link"
done
