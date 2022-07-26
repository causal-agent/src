#!/bin/sh
set -eu

readonly Host='temp.causal.agency'
readonly Root='/var/www'

temp=
temp() {
	temp=$(mktemp -d)
	trap 'rm -r "$temp"' EXIT
}

warn=
upload() {
	src=$1
	ext=${src##*.}
	name=$(printf '%x%s' "$(date +%s)" "$(openssl rand -hex 4)")
	url="${Host}/${name}.${ext}"
	scp -q "$src" "${Host}:${Root}/${url}"
	if test -n "$warn"; then
		test -n "$temp" || temp
		cat >"${temp}/warn.html" <<-EOF
			<!DOCTYPE html>
			<title>${warn}</title>
			<meta http-equiv="refresh" content="0;url=${name}.${ext}">
		EOF
		url="${Host}/${name}.html"
		scp -q "${temp}/warn.html" "${Host}:${Root}/${url}"
	fi
	echo "https://${url}"
}

uploadText() {
	temp
	cat >"${temp}/input.txt"
	upload "${temp}/input.txt"
}

uploadCommand() {
	temp
	echo "$ $1" >"${temp}/exec.txt"
	$SHELL -c "$1" >>"${temp}/exec.txt" 2>&1 || true
	upload "${temp}/exec.txt"
}

uploadHilex() {
	temp
	hilex -f html -o document,tab=4 "$@" >"${temp}/hilex.html"
	upload "${temp}/hilex.html"
}

uploadScreen() {
	temp
	if command -v screencapture >/dev/null; then
		screencapture -i "$@" "${temp}/capture.png"
	else
		scrot -s "$@" "${temp}/capture.png"
	fi
	pngo "${temp}/capture.png"
	upload "${temp}/capture.png"
}

uploadTerminal() {
	temp
	cat >"${temp}/term.html" <<-EOF
	<!DOCTYPE html>
	<meta charset="utf-8">
	<title>${1}</title>
	<style>
	$(scheme -s)
	</style>
	EOF
	ptee $SHELL -c "$1" >"${temp}/term.pty"
	shotty -Bs "${temp}/term.pty" >>"${temp}/term.html"
	upload "${temp}/term.html"
}

while getopts 'chstw:' opt; do
	case $opt in
		(c) fn=uploadCommand;;
		(h) fn=uploadHilex;;
		(s) fn=uploadScreen;;
		(t) fn=uploadTerminal;;
		(w) warn=$OPTARG;;
		(?) exit 1;;
	esac
done
shift $((OPTIND - 1))
[ $# -eq 0 ] && : ${fn:=uploadText}
: ${fn:=upload}

url=$($fn "$@")
printf '%s' "$url" | pbcopy || true
echo "$url"
