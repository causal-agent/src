#!/bin/sh
set -eu

mkdir -p static/preview static/thumbnail

resize() {
	local photo=$1 size=$2 output=$3
	if ! test -f $output; then
		# FIXME: convert complains about not understanding XML
		echo $output >&2
		convert $photo -auto-orient -thumbnail $size $output 2>/dev/null ||:
	fi
}

preview() {
	local photo=$1
	local preview=preview/${photo##*/}
	resize $photo 1500000@ static/$preview
	echo $preview
}

thumbnail() {
	local photo=$1
	local thumbnail=thumbnail/${photo##*/}
	resize $photo 60000@ static/$thumbnail
	echo $thumbnail
}

encode() {
	sed '
		s/&/\&amp;/g
		s/</\&lt;/g
		s/"/\&quot;/g
	' "$@"
}

page_title() {
	date -j -f '%F' $1 '+%B %e, %Y'
}

page_head() {
	local date=$1
	local title=$(page_title $date)
	local body lens film

	if test -f $date/body; then
		body=$(encode $date/body)
	fi
	if test -f $date/lens; then
		lens=$(
			sed '
				s,f/,ƒ/,
				s/\([0-9]\)-\([0-9]\)/\1-\2/g
			' $date/lens |
			encode
		)
	else
		lens=$(
			identify -format '%[EXIF:LensModel]' \
				$date/$(ls -1 $date | head -n 1) 2>/dev/null |
			sed '
				s/\([A-Z]\)\([0-9]\)/\1 \2/
				s,f/,ƒ/,
				s/\([0-9]\)-\([0-9]\)/\1–\2/g
			' |
			encode
		)
	fi
	if test -f $date/film; then
		film=$(encode $date/film)
	fi

	cat <<-EOF
	<!DOCTYPE html>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<link rel="alternate" type="application/atom+xml" href="../feed.atom">
	<title>${title}</title>
	<style>
	html { color: #bbb; background-color: black; font-family: monospace; }
	p { text-align: center; }
	figure { margin: 1em; padding-top: 0.5em; text-align: center; }
	img { max-width: calc(100vw - 2.5em); max-height: calc(100vh - 2.5em); }
	details { max-width: 78ch; margin: 0.5em auto; }
	</style>
	<h1>${title}</h1>
	<p>📷 ${body:-}${body:+ · }${lens}${film:+ 🎞️ }${film:-}</p>
	EOF
}

photo_info() {
	local photo=$1
	ExposureTime=
	FNumber=
	FocalLength=
	PhotographicSensitivity=
	eval $(
		identify -format '%[EXIF:*]' $photo 2>/dev/null |
		grep -E 'ExposureTime|FNumber|FocalLength|PhotographicSensitivity' |
		sed 's/^exif://'
	)
}

photo_id() {
	local photo=$1
	photo=${photo##*/}
	photo=${photo%%.*}
	echo $photo
}

page_photo() {
	local photo=$1 preview=$2 description=$3
	photo_info $photo
	cat <<-EOF
	<figure id="$(photo_id $photo)">
		<a href="${photo##*/}">
	EOF
	if test -f $description; then
		cat <<-EOF
			<img src="../${preview}" alt="$(encode $description)">
		EOF
	else
		cat <<-EOF
			<img src="../${preview}">
		EOF
	fi
	cat <<-EOF
		</a>
		<figcaption>
	EOF
	if test -n "${ExposureTime}"; then
		cat <<-EOF
			${ExposureTime} ·
			ƒ/$(bc -S 1 -e ${FNumber}) ·
			$(bc -e ${FocalLength}) mm ·
			${PhotographicSensitivity} ISO
		EOF
	fi
	if test -f $description; then
		cat <<-EOF
			<details>
				<summary>description</summary>
				$(encode $description)
			</details>
		EOF
	fi
	cat <<-EOF
		</figcaption>
	</figure>
	EOF
}

index_head() {
	cat <<-EOF
	<!DOCTYPE html>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<link rel="alternate" type="application/atom+xml" href="feed.atom">
	<title>Photos</title>
	<style>
	html { color: #bbb; background-color: black; font-family: sans-serif; }
	a { text-decoration: none; color: inherit; }
	</style>
	EOF
}

index_page() {
	local date=$1 root=${2:-}
	cat <<-EOF
	<h1><a href="${root}${root:+/}${date}/">$(page_title $date)</a></h1>
	EOF
}

index_photo() {
	local date=$1 photo=$2 thumbnail=$3 root=${4:-}
	cat <<-EOF
	<a href="${root}${root:+/}${date}/#$(photo_id $photo)">
		<img src="${root}${root:+/}${thumbnail}">
	</a>
	EOF
}

Root=https://photo.causal.agency

atom_head() {
	local updated=$(date -u '+%FT%TZ')
	cat <<-EOF
	<?xml version="1.0" encoding="utf-8"?>
	<feed xmlns="http://www.w3.org/2005/Atom">
	<title>Photos</title>
	<author><name>june</name><email>june@causal.agency</email></author>
	<link href="${Root}"/>
	<link rel="self" href="${Root}/feed.atom"/>
	<id>${Root}/</id>
	<updated>${updated}</updated>
	EOF
}

atom_entry_head() {
	local date=$1
	local updated=$(
		date -ju -f '%s' $(stat -f '%m' static/${date}/index.html) '+%FT%TZ'
	)
	cat <<-EOF
	<entry>
	<title>$(page_title $date)</title>
	<link href="${Root}/${date}/"/>
	<id>${Root}/${date}/</id>
	<updated>${updated}</updated>
	<content type="html">
	EOF
}

atom_entry_tail() {
	cat <<-EOF
	</content>
	</entry>
	EOF
}

atom_tail() {
	cat <<-EOF
	</feed>
	EOF
}

set --
for date in 20*; do
	mkdir -p static/${date}
	page=static/${date}/index.html
	if ! test -f $page; then
		echo $page >&2
		page_head $date >$page
		for photo in ${date}/*.[Jj][Pp][Gg]; do
			preview=$(preview $photo)
			if ! test -f static/${photo}; then
				ln $photo static/${photo}
			fi
			page_photo $photo $preview ${photo%.[Jj][Pp][Gg]}.txt >>$page
		done
	fi
	set -- $date "$@"
done

echo static/index.html >&2
index_head >static/index.html
echo static/feed.atom >&2
atom_head >static/feed.atom
for date; do
	index_page $date >>static/index.html
	atom_entry_head $date >>static/feed.atom
	for photo in ${date}/*.[Jj][Pp][Gg]; do
		thumbnail=$(thumbnail $photo)
		index_photo $date $photo $thumbnail >>static/index.html
		index_photo $date $photo $thumbnail $Root | encode >>static/feed.atom
	done
	atom_entry_tail >>static/feed.atom
done
atom_tail >>static/feed.atom
