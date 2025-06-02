#!/bin/sh
set -eu

Instance=https://tilde.zone
Root=${1:-static}

if ! test -f app.json; then
	echo 'No app.json!' >&2
	exit 1
fi
chmod 600 app.json

if ! test -f token.json; then
	client_id=$(jq -r .client_id app.json)
	client_secret=$(jq -r .client_secret app.json)
	echo "Please open ${Instance}/oauth/authorize?client_id=${client_id}&scope=write&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code"
	printf 'Enter code: '
	read -r code
	curl -Ss -X POST \
		-F 'grant_type=authorization_code' \
		-F "client_id=${client_id}" \
		-F "client_secret=${client_secret}" \
		-F 'redirect_uri=urn:ietf:wg:oauth:2.0:oob' \
		-F "code=${code}" \
		${Instance}/oauth/token >token.json
fi
chmod 600 token.json

access_token=$(jq -r .access_token token.json)

if ! test -f posted.txt; then
	touch posted.txt
fi

photo=$(
	find ${Root} -type f -path '*/0*/*.jpg' |
	sort | comm -13 posted.txt - | head -n 1
)
preview=${Root}/preview/${photo##*/}

media_id=$(
	curl -Ss -X POST \
		-H "Authorization: Bearer ${access_token}" \
		-F "file=@${preview}" \
		${Instance}/api/v2/media |
	jq -r .id
)

curl -Ss -X POST \
	-H "Authorization: Bearer ${access_token}" \
	-F "media_ids[]=${media_id}" \
	${Instance}/api/v1/statuses >/dev/null

echo ${photo} >>posted.txt
