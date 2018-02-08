ANY_BINS = atch dtch hnel pbcopy pbd pbpaste wake xx
BSD_BINS = jrp klon typo watch
MAC_BINS = gfxx-cocoa gfxx
LIN_BINS = bri fbatt fbclock gfxx-fb gfxx
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(MAC_BINS) $(LIN_BINS) gfxx-x11

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil -lz

any: .gitignore tags $(ANY_BINS)

bsd: any $(BSD_BINS)

mac: bsd $(MAC_BINS)

linux: any $(LIN_BINS)

atch: dtch
	ln -f dtch atch

gfxx-cocoa: gfxx.o gfcocoa.o
	$(CC) $(LDFLAGS) gfxx.o gfcocoa.o $(LDLIBS) -framework Cocoa -o $@

gfxx-fb: gfxx.o gfb.o
	$(CC) $(LDFLAGS) gfxx.o gfb.o $(LDLIBS) -o $@

gfxx-x11: gfxx.o gfx11.o
	$(CC) $(LDFLAGS) gfxx.o gfx11.o $(LDLIBS) -lX11 -o $@

gfxx:
	[ -f gfxx-fb ] && ln -s -f gfxx-fb gfxx || true
	[ -f gfxx-x11 ] && ln -s -f gfxx-x11 gfxx || true
	[ -f gfxx-cocoa ] && ln -s -f gfxx-cocoa gfxx || true

pbcopy pbpaste: pbd
	ln -f pbd $@

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f *.o $(ALL_BINS)

link:
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(ALL_BINS:%=~/.bin/%)

.gitignore: Makefile
	echo tags '*.o' $(ALL_BINS) | tr ' ' '\n' > .gitignore

tags: *.c
	ctags *.c

.PHONY: any bsd mac linux setuid clean link unlink
