#!/bin/sh
set -eu

find -L ~ -type l -lname "${PWD}/*" | while read -r link; do
	echo "$link"
	rm "$link"
done
