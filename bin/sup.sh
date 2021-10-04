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
unwrap() {
	sed '
		:x
		/=$/ {
			N
			s/=\n//g
			bx
		}
	'
}

asciinema() {
	echo 'Fetching CSRF token...'
	jar=$(mktemp -t sup)
	trap 'rm "${jar}"' EXIT
	csrf=$(
		curl -Ss -c "${jar}" 'https://asciinema.org/login/new' |
		sed -n 's/.*name="_csrf_token".*value="\([^"]*\)".*/\1/p'
	)
	echo 'Submitting form...'
	curl -Ss -X POST -b "${jar}" \
		-F "_csrf_token=${csrf}" -F "login[email]=${email}" \
		'https://asciinema.org/login' \
		>/dev/null
	echo 'Waiting for email...'
	url=$(
		git fetch-email -i -M Trash \
			-F 'hello@asciinema.org' -T "${email}" \
			-S 'Login to asciinema.org' |
		grep -m 1 '^https://asciinema\.org/session/new'
	)
	open "${url}"
}

bugzilla() {
	echo 'Fetching CSRF token...'
	csrf=$(
		curl -Ss "${bugzillaBase}/" |
		sed -n '
			/name="token"/N
			s/.*name="token"[[:space:]]*value="\([^"]*\)".*/\1/p
		' | head -n 1
	)
	echo 'Submitting form...'
	curl -Ss -X POST \
		-F "loginname=${email}" -F "token=${csrf}" -F 'a=reqpw' \
		"${bugzillaBase}/token.cgi" \
		>/dev/null
	echo 'Waiting for email...'
	token=$(
		git fetch-email -i -M Trash \
			-F "${bugzillaFrom}" -T "${email}" \
			-S 'Bugzilla Change Password Request' |
		sed -n 's/.*t=3D\([^&]*\).*/\1/p' |
		head -n 1
	)
	password=$(generate)
	echo 'Setting password...'
	curl -Ss -X POST \
		-F "t=${token}" -F 'a=chgpw' \
		-F "password=${password}" -F "matchpassword=${password}" \
		"${bugzillaBase}/token.cgi" \
		>/dev/null
	copy "${password}"
	open "${bugzillaBase}/"
}

freebsdbugzilla() {
	bugzillaBase='https://bugs.freebsd.org/bugzilla'
	bugzillaFrom='bugzilla-noreply@freebsd.org'
	bugzilla
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
	token=$(curl -ISs --url "${url}" | sed -n 's/.*[?]token=\([^&]*\).*/\1/p')
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

liberapay() {
	echo 'Fetching CSRF token...'
	csrf=$(
		curl -Ss 'https://liberapay.com/sign-in' |
		sed -n 's/.*name="csrf_token".*value="\([^"]*\)".*/\1/p'
	)
	echo 'Submitting form...'
	curl -Ss -X POST \
		-b "csrf_token=${csrf}" -F "csrf_token=${csrf}" \
		-F "log-in.id=${email}" \
		'https://liberapay.com/sign-in' \
		>/dev/null
	echo 'Waiting for email...'
	url=$(
		git fetch-email -i -M Trash \
			-F 'support@liberapay.com' -T "${email}" \
			-S 'Log in to Liberapay' |
		grep -m 1 '^https://liberapay\.com/'
	)
	open "${url}"
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
		sed -n 's|^https://.*[?]token=\([^&]*\).*|\1|p'
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

patreon() {
	readonly patreonAPI='https://www.patreon.com/api'
	echo 'Submitting form...'
	curl -Ss -X POST -d @- \
		-H 'Content-Type: application/vnd.api+json' \
		"${patreonAPI}/auth/forgot-password?json-api-version=1.0" <<-EOF
		{"data":{"email":"${email}"}}
		EOF
	echo 'Waiting for email...'
	url=$(
		git fetch-email -i -M Trash \
			-F 'password@patreon.com' -T "${email}" \
			-S 'Patreon Password Reset' |
		unwrap |
		grep -o -m 1 'https://email[.]mailgun[.]patreon[.]com/.*'
	)
	echo 'Fetching token...'
	location=$(curl -ISs --url "${url}" | grep -i '^Location: ' | tr -d '\r')
	u=$(echo "${location}" | sed 's/.*[?&]u=\([^&]*\).*/\1/')
	sec=$(echo "${location}" | sed 's/.*[?&]sec=\([^&]*\).*/\1/')
	password=$(generate)
	echo 'Setting password...'
	curl -Ss -X POST -d @- \
		-H 'Content-Type: application/vnd.api+json' \
		"${patreonAPI}/auth/forgot-password/change?json-api-version=1.0" <<-EOF
		{
			"data":{
				"user_id":"${u}",
				"security_token":"${sec}",
				"password":"${password}"
			}
		}
		EOF
	copy "${password}"
	open 'https://www.patreon.com/login'
}

tildenews() {
	lobstersBase='https://tilde.news'
	lobstersFrom='nobody@tilde.news'
	lobsters
}

$service
