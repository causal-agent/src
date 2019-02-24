#!/bin/sh
set -eu

readonly Host='temp.causal.agency'

upload() {
	local src ext ts rand url
	src=$1
	ext=${src##*.}
	ts=$(date +'%s')
	rand=$(openssl rand -hex 4)
	url=$(printf '%s/%x%s.%s' "$Host" "$ts" "$rand" "$ext")
	scp -q "$src" "${Host}:/usr/local/www/${url}"
	echo "https://${url}"
}

temp() {
	temp=$(mktemp -d)
	trap "rm -r '$temp'" EXIT
}

uploadText() {
	temp
	cat > "${temp}/input.txt"
	upload "${temp}/input.txt"
}

uploadHi() {
	temp
	hi -f html -o document,anchor,tab=4 "$@" > "${temp}/hi.html"
	upload "${temp}/hi.html"
}

uploadScreen() {
	temp
	screencapture -i "$@" "${temp}/capture.png"
	pngo "${temp}/capture.png" || true
	upload "${temp}/capture.png"
}

args=$(setopt 'hs' "$@")
eval set -- "$args"
for opt; do
	case "$opt" in
		(-h) shift; fn=uploadHi;;
		(-s) shift; fn=uploadScreen;;
		(--) shift; break;;
	esac
done
[ $# -eq 0 ] && : ${fn:=uploadText}
: ${fn:=upload}

url=$($fn "$@")
printf '%s' "$url" | pbcopy || true
echo "$url"
