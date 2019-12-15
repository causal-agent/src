#!/bin/sh
set -eu

readonly GitURL='https://git.causal.agency/src/tree/bin'

src=$1
man=${2:-}

./hi -f html -o document,tab=4 -n "$src" /dev/null | sed '/<pre/d'
cat <<- EOF
	<code><a href="${GitURL}/${src}">${src} in git</a></code>
EOF
[ -f "$man" ] && man -P cat "${PWD}/${man}" | ./ttpre
./hi -f html -o anchor "$src"
