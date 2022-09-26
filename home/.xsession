. ~/.profile
export LC_CTYPE=en_US.UTF-8

xset r rate 175 m 5/4 0
xmodmap ~/.config/X/modmap
xrdb -load ~/.config/X/resources

fg=998D6B
command -v scheme && fg=$(scheme -p $(jot -r 1 1 8))
xsetroot -bitmap /usr/X11R6/include/X11/bitmaps/escherknot \
	-bg '#14130E' -fg "#${fg}"

xterm -name clock -geometry 14x1-0+0 -sl 0 -e clock &
exec cwm -c ~/.config/cwm/cwmrc
