_PATH=$PATH PATH=
path() { [ -d "$1" ] && PATH="${PATH}${PATH:+:}${1}"; }
for prefix in '' /usr/local /usr/pkg /usr /opt/pkg ~/.local; do
	path "${prefix}/sbin"
	path "${prefix}/bin"
done
path /usr/games

export PAGER=less
export LESS=FRX
export EDITOR=nvim
export MANPAGER="nvim -c 'set ft=man' -"
export MANSECT=2:3:1:8:6:5:7:4:9
export CLICOLOR=1
export GPG_TTY=$(tty)
export NETHACKOPTIONS='pickup_types:$!?+/=, color, DECgraphics'

type nvim >/dev/null || EDITOR=vim
[ -e /usr/share/mk/sys.mk ] || export CFLAGS=-O
cd

export ENV=~/.shrc
