ANY_BINS = atch dtch glitch hnel modem open pbcopy pbd pbpaste pngo scheme wake xx
BSD_BINS = klon watch
LIN_BINS = bri fbatt fbclock
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(LIN_BINS)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -lm -lutil -lz

any: .gitignore tags $(ANY_BINS)

bsd: any $(BSD_BINS)

linux: any $(LIN_BINS)

.gitignore: Makefile
	echo tags $(ALL_BINS) scheme.png | tr ' ' '\n' > .gitignore

tags: *.c
	ctags -w *.c

atch: dtch
	ln -f dtch atch

open pbcopy pbpaste: pbd
	ln -f pbd $@

scheme.png: scheme
	./scheme -t -g > scheme.png

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f tags $(ALL_BINS)

link:
	mkdir -p ~/.local/bin
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.local/bin

unlink:
	rm -f $(ALL_BINS:%=~/.local/bin/%)
