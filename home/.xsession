. ~/.profile
xset r rate 175 m 4
xmodmap ~/.config/X/modmap
xrdb -load ~/.config/X/resources
xsetroot -bitmap /usr/X11R6/include/X11/bitmaps/xsnow \
	-bg rgb:7A/49/55 -fg rgb:16/15/10
cwm -c ~/.config/cwm/cwmrc
