#!/bin/bash

# Remove symbolic links in ~ to files that no longer exist.

set -e

paths=$(find -L ~ -type l -lname "$PWD/*")

for path in $paths; do
  rm $path
  echo $path
done
