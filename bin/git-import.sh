#!/bin/sh
set -eu

: ${CURL:=curl}

die() {
	echo "$0: $*" >&2
	exit 1
}

strip=1
message=

OPTS_SPEC="\
git import [<options>] <branch> <file>
--
m,message=! create a commit with the specified message
p,strip= remove the specified number of leading path elements
"
eval "$(echo "$OPTS_SPEC" | git rev-parse --parseopt -- "$@")"

while [ $# -gt 0 ]; do
	opt=$1
	shift
	case "$opt" in
		(-m) message=$1; shift;;
		(-p) strip=$1; shift;;
		(--no-strip) strip=0;;
		(--) break;;
	esac
done
[ $# -gt 0 ] || die "branch name required"
[ $# -gt 1 ] || die "file or url required"
branch=$1
tarball=$2
: ${message:="Import ${tarball##*/}"}

check_index() {
	if ! git diff-index --quiet HEAD; then
		die "working tree has modifications"
	fi
}

check_head() {
	local head full
	head=$(git rev-parse --symbolic-full-name HEAD)
	full=$(git rev-parse --verify --quiet --symbolic-full-name "$1") || return 0
	if [ "$full" = "$head" ]; then
		die "branch (${full}) cannot be HEAD (${head})"
	fi
}

extract_tarball() {
	local url strip tarball tmpdir
	url=$1
	strip=$2
	if test -f "$url"; then
		tarball=$url
	else
		tarball=$(mktemp)
		trap 'rm -f "$tarball"' EXIT
		$CURL -L -o "$tarball" "$url"
	fi
	tmpdir=$(mktemp -d)
	tar -x -f "$tarball" -C "$tmpdir" --strip-components "$strip"
	echo "$tmpdir"
}

create_tree() {
	local root fullpath path blob mode tree
	root=$1
	git read-tree --empty
	find "$root" -type f | while read -r fullpath; do
		path=${fullpath#${root}/}
		blob=$(git hash-object -w --path "$path" "$fullpath")
		mode=100644
		if test -L "$fullpath"; then
			mode=120000
		elif test -x "$fullpath"; then
			mode=100755
		fi
		git update-index --add --cacheinfo "$mode,$blob,$path"
	done
	tree=$(git write-tree)
	git read-tree HEAD
	echo "$tree"
}

create_commit() {
	local parent tree message
	parent=$1
	tree=$2
	message=$3
	git commit-tree ${parent:+-p ${parent}} -m "$message" "$tree"
}

update_branch() {
	local branch parent commit
	branch=$1
	parent=$2
	commit=$3
	git update-ref "refs/heads/$branch" "$commit" "$parent"
}

check_index
check_head "$branch"
root=$(extract_tarball "$tarball" "$strip")
tree=$(create_tree "$root")
rm -fr "$root"
parent=$(git rev-parse --verify --quiet "$branch" || :)
commit=$(create_commit "$parent" "$tree" "$message")
update_branch "$branch" "$parent" "$commit"
git --no-pager show --stat "$commit"
