#!/bin/sh
set -eu

readonly GitURL='https://git.causal.agency/src/tree/bin'

src=$1
man=${2:-}

cat <<EOF
<!DOCTYPE html>
<title>${src}</title>
<style>
$(./scheme -st)
html {
	font-family: monospace;
	color: var(--ansi17);
	background-color: var(--ansi16);
}
a { color: var(--ansi4); }
a:visited { color: var(--ansi5); }
pre.hi {
	-moz-tab-size: 4;
	tab-size: 4;
}
.hi.Keyword { color: var(--ansi7); }
.hi.Macro { color: var(--ansi2); }
.hi.Tag { color: inherit; text-decoration: underline; }
.hi.Tag:target { color: var(--ansi11); outline: none; }
.hi.String { color: var(--ansi6); }
.hi.Format { color: var(--ansi14); }
.hi.Interp { color: var(--ansi3); }
.hi.Comment { color: var(--ansi4); }
.hi.Todo { color: var(--ansi12); }
.hi.DiffOld { color: var(--ansi1); }
.hi.DiffNew { color: var(--ansi2); }
</style>
<a href="${GitURL}/${src}">${src} in git</a>
EOF

[ -f "$man" ] && man -P cat "${PWD}/${man}" | ./ttpre
./hi -f html -o anchor "$src"
