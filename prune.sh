#!/bin/sh
set -e -u

find -L ~ -type l -lname "$PWD/*" | while read -r linkPath; do
	rm "$linkPath"
	echo "$linkPath"
done
