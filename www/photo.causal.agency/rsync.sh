#!/bin/sh
set -eu

sh generate.sh
rsync -av static/ scout:/var/www/photo.causal.agency
