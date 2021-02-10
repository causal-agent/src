. ~/.profile
export LC_CTYPE=C.UTF-8

xset r rate 175 m 5/4 0
xmodmap ~/.config/X/modmap
xrdb -load ~/.config/X/resources
xsetroot -bitmap /usr/X11R6/include/X11/bitmaps/xsnow \
	-bg rgb:14/13/0E -fg rgb:7A/49/55 

sctd &
xterm -name clock -geometry 14x1-0+0 -sl 0 -e clock &
exec cwm -c ~/.config/cwm/cwmrc
