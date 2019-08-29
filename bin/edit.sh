#!/bin/sh
set -eu

script=$(mktemp)
trap "rm -f '$script'" EXIT

sed "s/.*/${1:-: &}/" >> "$script"
$EDITOR "$script"
sh -eux "$script"
