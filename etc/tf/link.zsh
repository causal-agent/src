#!/usr/bin/env zsh
set -o errexit -o nounset

tf="$HOME/Library/Application Support/Steam/steamapps/common/Team Fortress 2/tf"
for cfg in cfg/*.cfg; do
	ln -s -f "$PWD/$cfg" "$tf/$cfg"
done
