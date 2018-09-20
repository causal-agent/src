#!/bin/sh
set -e -u

updated=$(date -u '+%FT%TZ')
cat <<EOF
<?xml version="1.0" encoding="utf-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
<title>Causal Agency</title>
<author><name>June</name><email>june@causal.agency</email></author>
<link href="https://text.causal.agency"/>
<id>https://text.causal.agency/</id>
<updated>${updated}</updated>
EOF
for entry in *.7; do
	url="https://text.causal.agency/${entry%.7}.txt"
	title=$(grep '^\.Nm' "$entry" | cut -c 5-)
	summary=$(grep '^\.Nd' "$entry" | cut -c 5-)
	updated=$(date -j -u -f '%s' "$(stat -f '%m' "$entry")" '+%FT%TZ')
	cat <<EOF
	<entry>
	<title>${title}</title>
	<summary>${summary}</summary>
	<link href="${url}"/>
	<id>${url}</id>
	<updated>${updated}</updated>
	</entry>
EOF
done
echo '</feed>'
