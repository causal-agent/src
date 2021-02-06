_PATH=$PATH PATH=
path() { [ -d "$1" ] && PATH="${PATH}${PATH:+:}${1}"; }
for prefix in '' /usr/local /opt/local /usr ~/.local; do
	path "${prefix}/sbin"
	path "${prefix}/bin"
done
path /usr/X11R6/bin /usr/games

export EDITOR=vi
type nvi >/dev/null && EDITOR=nvi
export EXINIT='set ai ic sm sw=4 ts=4 para=BlBdPpIt sect=ShSs | map gg 1G'
export PAGER=less
export LESS=FRXix4
export CLICOLOR=1
export MANSECT=2:3:1:8:6:5:7:4:9
export LESS_TERMCAP_us=$(tput sitm)
export LESS_TERMCAP_ue=$(tput ritm)
export NETHACKOPTIONS='pickup_types:$!?+/=, color, DECgraphics'

[ -e /usr/share/mk/sys.mk ] || export CFLAGS=-O
[ -d /usr/home ] && cd

export ENV=~/.shrc
