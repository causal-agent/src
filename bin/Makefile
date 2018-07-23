ANY_BINS = atch brot dtch gfxx glitch hnel modem pbcopy pbd pbpaste pngo scheme wake xx
BSD_BINS = klon watch
LIN_BINS = bri fbatt fbclock
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(LIN_BINS)
GFX ?= none

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -ledit -lm -lcurses -lutil -lz
LDLIBS_cocoa = -framework Cocoa
LDLIBS_x11 = -lX11

any: .gitignore tags $(ANY_BINS)

bsd: any $(BSD_BINS)

linux: any $(LIN_BINS)

.gitignore: Makefile
	echo '*.o' tags $(ALL_BINS) scheme.png | tr ' ' '\n' > .gitignore

tags: *.c
	ctags -w *.c

atch: dtch
	ln -f dtch atch

brot: brot.o gfx/$(GFX).o
	$(CC) $(LDFLAGS) brot.o gfx/$(GFX).o $(LDLIBS) $(LDLIBS_$(GFX)) -o $@

gfxx: gfxx.o gfx/$(GFX).o
	$(CC) $(LDFLAGS) gfxx.o gfx/$(GFX).o $(LDLIBS) $(LDLIBS_$(GFX)) -o $@

pbcopy pbpaste: pbd
	ln -f pbd $@

scheme.png: scheme
	./scheme -t -g > scheme.png

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f tags *.o gfx/*.o $(ALL_BINS)

link:
	mkdir -p ~/.local/bin
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.local/bin

unlink:
	rm -f $(ALL_BINS:%=~/.local/bin/%)
