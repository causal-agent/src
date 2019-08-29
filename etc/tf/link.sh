#!/bin/sh
set -eu

tf="${HOME}/Library/Application Support/Steam/steamapps/common/Team Fortress 2/tf"
for cfg in cfg/*.cfg; do
	ln -fs "${PWD}/${cfg}" "${tf}/${cfg}"
done
