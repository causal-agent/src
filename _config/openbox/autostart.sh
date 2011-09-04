thunar --daemon &

# Compositing
#xcompmgr &
xcompmgr -c -f -D 3 -C &

# Wallpaper
nitrogen --restore

# Panels/Docks
lxpanel &
docky &

# Tray apps
parcellite &
volwheel &

# Conky
conky &

# Other daemons
/home/curtis/code/c/keycounter/keycounter -f -d -p /home/curtis/.keycounter.pid /home/curtis/.keycount &
