#!/bin/sh
set -eu

readonly Root='https://text.causal.agency'

updated=$(date -u '+%FT%TZ')
cat <<-EOF
	<?xml version="1.0" encoding="utf-8"?>
	<feed xmlns="http://www.w3.org/2005/Atom">
	<title>Causal Agency</title>
	<author><name>June</name><email>june@causal.agency</email></author>
	<link href="${Root}"/>
	<link rel="self" href="${Root}/feed.atom"/>
	<id>${Root}/</id>
	<updated>${updated}</updated>
EOF

encode() {
	sed '
		s/&/\&amp;/g
		s/</\&lt;/g
		s/"/\&quot;/g
	' "$@"
}

set -- *.txt
shift $(( $# - 20 ))
for txt; do
	entry="${txt%.txt}.7"
	date=$(grep '^[.]Dd' "$entry" | cut -c 5-)
	title=$(grep '^[.]Nm' "$entry" | cut -c 5- | encode)
	summary=$(grep '^[.]Nd' "$entry" | cut -c 5- | encode)
	published=$(date -ju -f '%B %d, %Y %T' "${date} 00:00:00" '+%FT%TZ')
	mtime=$(stat -f '%m' "$entry")
	updated=$(date -ju -f '%s' "$mtime" '+%FT%TZ')
	cat <<-EOF
		<entry>
		<title>${title}</title>
		<summary>${summary}</summary>
		<link href="${Root}/${txt}"/>
		<id>${Root}/${txt}</id>
		<published>${published}</published>
		<updated>${updated}</updated>
		<content type="xhtml">
		<div xmlns="http://www.w3.org/1999/xhtml">
	EOF
	printf '<pre>'
	encode "$txt"
	cat <<-EOF
		</pre>
		</div>
		</content>
		</entry>
	EOF
done

echo '</feed>'
