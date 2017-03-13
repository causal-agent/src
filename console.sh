#!/usr/bin/env zsh

# Set the console color palette.

color() {
  print -n "\e]P${1}${2}"
}

color 0 '1d2021'
color 1 'cc241d'
color 2 '98991a'
color 3 'd89a22'
color 4 '468589'
color 5 'b16286'
color 6 '689e6a'
color 7 'a99a84'

color 8 '938374'
color 9 'fc4935'
color a 'b8ba26'
color b 'fbbe2f'
color c '83a699'
color d 'd4869c'
color e '8fc17d'
color f 'eddbb3'

clear
