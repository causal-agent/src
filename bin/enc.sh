#!/bin/sh
set -eu

readonly Command='openssl enc -ChaCha20 -pbkdf2'

base64=
stdout=false
mode=encrypt
force=false

while getopts 'acdef' opt; do
	case $opt in
		(a) base64=-a;;
		(c) stdout=true;;
		(d) mode=decrypt;;
		(e) mode=encrypt;;
		(f) force=true;;
		(?) exit 1;;
	esac
done
shift $((OPTIND - 1))

confirm() {
	$force && return 0
	while :; do
		printf '%s: overwrite %s? [y/N] ' "$0" "$1" >&2
		read -r confirm
		case "$confirm" in
			(Y*|y*) return 0;;
			(N*|n*|'') return 1;;
		esac
	done
}

encrypt() {
	if test -z "${1:-}"; then
		$Command -e $base64
	elif $stdout; then
		$Command -e $base64 -in "$1"
	else
		input=$1
		output="${1}.enc"
		if test -e "$output" && ! confirm "$output"; then
			return
		fi
		$Command -e $base64 -in "$input" -out "$output"
	fi
}

decrypt() {
	if test -z "${1:-}"; then
		$Command -d $base64
	elif $stdout || [ "${1%.enc}" = "$1" ]; then
		$Command -d $base64 -in "$1"
	else
		input=$1
		output=${1%.enc}
		if test -e "$output" && ! confirm "$output"; then
			return
		fi
		$Command -d $base64 -in "$input" -out "$output"
	fi
}

for input; do
	$mode "$input"
done
if [ $# -eq 0 ]; then
	$mode
fi
