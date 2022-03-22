_PATH=$PATH PATH=
path() { test -d "$1" && PATH="${PATH}${PATH:+:}${1}"; }
for prefix in '' /usr/local /opt/local /usr ~/.local ~/.cargo; do
	path "${prefix}/sbin"
	path "${prefix}/bin"
done
path /usr/X11R6/bin
path /usr/games
export MANPATH=:~/.local/share/man

export EDITOR=vi
command -v nvi >/dev/null && EDITOR=nvi
export EXINIT='set ai extended iclower sm sw=4 ts=4 para=BlBdPpIt sect=ShSs
map gg 1G'
export PAGER=less
export LESS=FRXix4
export CLICOLOR=1
export MANSECT=2:3:1:8:6:5:7:4:9
export NETHACKOPTIONS='pickup_types:$!?+/=, color, DECgraphics'
command -v diff-highlight >/dev/null &&
export GIT_PAGER="diff-highlight | $PAGER"

test -e /usr/share/mk/sys.mk || export CFLAGS=-O
test -d /usr/home && cd

test -f ~/.profile.local && . ~/.profile.local

export ENV=~/.shrc
