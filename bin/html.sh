#!/bin/sh
set -eu

readonly GiteaURL='https://code.causal.agency/june/src/src/branch/master/bin'

src=$1
man=${2:-}

./hi -f html -o document,tab=4 -n "${src}" /dev/null | sed '/<pre/d'
cat <<- EOF
	<code><a href="${GiteaURL}/${src}">${src} in git</a></code>
EOF
[ -f "${man}" ] && man -P cat "${PWD}/${man}" | ./ttpre
./hi -f html -o anchor "${src}"
