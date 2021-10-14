#!/bin/sh
set -eu

readonly GitURL='https://git.causal.agency/src/tree/bin'

man=$1
shift
title=${man##*/}
title=${title%.[1-9]}

cat <<EOF
<!DOCTYPE html>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>${title}</title>
<style>
html { line-height: 1.25em; font-family: monospace; }
body { max-width: 80ch; margin: 1em auto; padding: 0 1ch; }

table.head, table.foot { width: 100%; }
td.head-rtitle, td.foot-os { text-align: right; }
td.head-vol { text-align: center; }
div.Pp { margin: 1ex 0ex; }
div.Nd, div.Bf, div.Op { display: inline; }
span.Pa, span.Ad { font-style: italic; }
span.Ms { font-weight: bold; }
dl.Bl-diag > dt { font-weight: bold; }
code.Nm, code.Fl, code.Cm, code.Ic, code.In, code.Fd, code.Fn,
code.Cd { font-weight: bold; font-family: inherit; }

table { border-collapse: collapse; }
table.Nm code.Nm { padding-right: 1ch; }
table.foot { margin-top: 1em; }

ul.index { padding: 0; }
ul.index li { display: inline; list-style-type: none; }
pre { -moz-tab-size: 4; tab-size: 4; }

$(./scheme -st)
html { background-color: var(--ansi16); color: var(--ansi17); }
a { color: var(--ansi4); }
a:visited { color: var(--ansi5); }
a.permalink, a.tag { color: var(--ansi3); text-decoration: none; }
a.permalink > code:target, *:target > a.permalink,
a.tag:target { color: var(--ansi11); }
pre .Ke { color: var(--ansi7); }
pre .Ma { color: var(--ansi2); }
pre .Co { color: var(--ansi4); }
pre .St { color: var(--ansi6); }
pre .Fo { color: var(--ansi14); }
pre .Su { color: var(--ansi1); }
</style>
EOF

opts=fragment
[ "${man}" = "README.7" ] && opts=${opts},man=%N.html
mandoc -T html -O ${opts} "${man}"

for src; do
	cat <<-EOF
	<p>
	<a href="${GitURL}/${src}">${src} in git</a>
	EOF
	./htagml -x -f htmltags "${src}"
	./hilex -t -f html "${src}" | ./htagml -ip -f htmltags "${src}"
done
