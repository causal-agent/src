#!/usr/bin/env zsh

# Pull latest versions of vendored files.

set -o errexit -o nounset -o pipefail

pull() {
  curl -s "https://raw.githubusercontent.com/$2" -o "$1"
  echo "$1"
}

pull {.config/nvim,tpope/vim-pathogen/master}/autoload/pathogen.vim
