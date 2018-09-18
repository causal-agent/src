#!/bin/sh
set -e -u

updated=$(date -u '+%FT%TZ')
echo '<?xml version="1.0" encoding="utf-8"?>'
echo '<feed xmlns="http://www.w3.org/2005/Atom">'
echo '<title>Causal Agency</title>'
echo '<author><name>June</name><email>june@causal.agency</email></author>'
echo '<link href="https://text.causal.agency"/>'
echo '<id>https://text.causal.agency/</id>'
echo "<updated>${updated}</updated>"
for entry in *.7; do
	url="https://text.causal.agency/${entry%.7}.txt"
	title=$(grep '^\.Nm' "$entry" | cut -c 5-)
	summary=$(grep '^\.Nd' "$entry" | cut -c 5-)
	updated=$(date -j -u -f '%s' "$(stat -f '%m' "$entry")" '+%FT%TZ')
	echo '<entry>'
	echo "<title>${title}</title>"
	echo "<summary>${summary}</summary>"
	echo "<link href=\"https://text.causal.agency/${entry%.7}.txt\"/>"
	echo "<id>${url}</id>"
	echo "<updated>${updated}</updated>"
	echo '</entry>'
done
echo '</feed>'
