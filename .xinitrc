export BROWSER=chromium

xrdb ~/.Xresources &
xautolock -locker slimlock -time 30 &
unclutter -idle 3 -noevents &
start-pulseaudio-x11 &
gnome-keyring-daemon &

mpd &
mpdscribble &
mpDris2 &

# Keyboard stuff
xmodmap -e "clear Lock" -e "keysym Caps_Lock = Escape" # Caps Lock -> Esc
xmodmap -e "remove mod1 = Alt_R" -e "keysym Alt_R = Multi_key" # Right Alt -> Compose
xmodmap -e "remove mod1 = Alt_L" -e "remove mod4 = Super_L" -e "add mod1 = Super_L" -e "add mod4 = Alt_L" # Swap Super and Alt

SESSION=openbox-session
[ $1 ] && SESSION=$1
exec ck-launch-session dbus-launch $SESSION
