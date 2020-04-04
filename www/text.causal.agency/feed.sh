#!/bin/sh
set -eu

readonly Root='https://text.causal.agency'

updated=$(date -u '+%FT%TZ')
cat <<- EOF
	<?xml version="1.0" encoding="utf-8"?>
	<feed xmlns="http://www.w3.org/2005/Atom">
	<title>Causal Agency</title>
	<author><name>June</name><email>june@causal.agency</email></author>
	<link href="${Root}"/>
	<id>${Root}</id>
	<updated>${updated}</updated>
EOF

for entry in *.7; do
	url="${Root}/${entry%.7}.txt"
	date=$(grep '^[.]Dd' "$entry" | cut -c 5-)
	title=$(grep '^[.]Nm' "$entry" | cut -c 5-)
	summary=$(grep '^[.]Nd' "$entry" | cut -c 5-)
	published=$(date -ju -f '%B %d, %Y %T' "${date} 00:00:00" '+%FT%TZ')
	mtime=$(stat -f '%m' "$entry")
	updated=$(date -ju -f '%s' "$mtime" '+%FT%TZ')
	cat <<- EOF
		<entry>
		<title>${title}</title>
		<summary>${summary}</summary>
		<link href="${url}"/>
		<id>${url}</id>
		<published>${published}</published>
		<updated>${updated}</updated>
		<content type="text/plain" src="${url}"/>
		</entry>
	EOF
done

echo '</feed>'
