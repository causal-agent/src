ANY_BINS = atch dtch gfxx hnel pbcopy pbd pbpaste wake xx
BSD_BINS = jrp klon typo watch
LIN_BINS = bri fbatt fbclock
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(LIN_BINS)
GFX = none

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil -lz
LDLIBS_cocoa = $(LDLIBS) -framework Cocoa
LDLIBS_x11 = $(LDLIBS) -lX11

any: .gitignore tags $(ANY_BINS)

bsd: any $(BSD_BINS)

linux: any $(LIN_BINS)

atch: dtch
	ln -f dtch atch

gfxx: gfxx.o gfx/$(GFX).o
	$(CC) $(LDFLAGS) gfxx.o gfx/$(GFX).o $(LDLIBS_$(GFX)) -o $@

pbcopy pbpaste: pbd
	ln -f pbd $@

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f *.o gfx/*.o $(ALL_BINS)

link:
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(ALL_BINS:%=~/.bin/%)

.gitignore: Makefile
	echo tags '*.o' $(ALL_BINS) | tr ' ' '\n' > .gitignore

tags: *.c
	ctags *.c

.PHONY: any bsd linux setuid clean link unlink
