#!/bin/sh
set -eu

service=$1
email=${2:-$(git config fetchemail.imapUser)}

generate() {
	openssl rand -base64 33
}
copy() {
	printf '%s' "$1" | pbcopy
}

discogs() {
	echo 'Submitting form...'
	curl -Ss -X POST \
		-F "email=${email}" -F 'Action.EmailResetInstructions=submit' \
		'https://www.discogs.com/users/forgot_password' \
		>/dev/null
	echo 'Waiting for email...'
	url=$(
		git fetch-email -i -M Trash \
			-F 'noreply@discogs.com' -T "${email}" \
			-S 'Discogs Account Password Reset Instructions' |
		sed -n 's/^To proceed, follow the instructions here: \(.*\)/\1/p'
	)
	echo 'Fetching token...'
	token=$(curl -ISs "${url}" | sed -n 's/.*[?]token=\([^&]*\).*/\1/p')
	password=$(generate)
	echo 'Setting password...'
	curl -Ss -X POST \
		-F "token=${token}" \
		-F "password0=${password}" -F "password1=${password}" \
		-F 'Action.ChangePassword=submit' \
		'https://www.discogs.com/users/forgot_password' \
		>/dev/null
	copy "${password}"
	open 'https://discogs.com/login'
}

lobsters() {
	: ${lobstersBase:=https://lobste.rs}
	: ${lobstersFrom:=nobody@lobste.rs}
	echo 'Fetching CSRF token...'
	csrf=$(
		curl -Ss "${lobstersBase}/login/forgot_password" |
		sed -n 's/.*name="authenticity_token" value="\([^"]*\)".*/\1/p'
	)
	echo 'Submitting form...'
	curl -Ss -X POST \
		-F "authenticity_token=${csrf}" \
		-F "email=${email}" -F 'commit=submit' \
		"${lobstersBase}/login/reset_password" \
		>/dev/null
	echo 'Waiting for email...'
	token=$(
		git fetch-email -i -M Trash \
			-F "${lobstersFrom}" -T "${email}" \
			-S 'Reset your password' |
		sed -n 's|^https://.*[?]token=\(.*\)|\1|p'
	)
	echo 'Fetching CSRF token...'
	csrf=$(
		curl -Ss "${lobstersBase}/login/set_new_password?token=${token}" |
		sed -n 's/.*name="authenticity_token" value="\([^"]*\)".*/\1/p'
	)
	password=$(generate)
	echo 'Setting password...'
	curl -Ss -X POST \
		-F "authenticity_token=${csrf}" -F "token=${token}" \
		-F "password=${password}" -F "password_confirmation=${password}" \
		-F 'commit=submit' \
		"${lobstersBase}/login/set_new_password" \
		>/dev/null
	copy "${password}"
	open "${lobstersBase}/login"
}

$service
