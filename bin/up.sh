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

uploadCommand() {
	temp
	echo "$ $*" > "${temp}/exec.txt"
	"$@" >> "${temp}/exec.txt" 2>&1 || true
	upload "${temp}/exec.txt"
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

uploadTerminal() {
	temp
	cat > "${temp}/term.html" <<-EOF
	<!DOCTYPE html>
	<title>${1}</title>
	<style>
	$(scheme -s)
	</style>
	EOF
	ptee "$@" | shotty -Bs >> "${temp}/term.html"
	upload "${temp}/term.html"
}

while getopts 'chst' opt; do
	case "$opt" in
		(c) fn=uploadCommand;;
		(h) fn=uploadHi;;
		(s) fn=uploadScreen;;
		(t) fn=uploadTerminal;;
		(?) exit 1;;
	esac
done
shift $(($OPTIND - 1))
[ $# -eq 0 ] && : ${fn:=uploadText}
: ${fn:=upload}

url=$($fn "$@")
printf '%s' "$url" | pbcopy || true
echo "$url"
