#!/bin/sh
set -eu

read -r first rest || :
[ "${first}" != "${first#C}" ] && first='C.'
echo "${first}" "${rest}"
