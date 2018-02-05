ANY_BINS = atch dtch hnel pbcopy pbd pbpaste wake xx
BSD_BINS = jrp klon typo watch
MAC_BINS = gfxx-cocoa gfxx
LIN_BINS = bri fbatt fbclock gfxx-fb gfxx
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(MAC_BINS) $(LIN_BINS)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil -lz

any: .gitignore $(ANY_BINS)

bsd: any $(BSD_BINS)

mac: bsd $(MAC_BINS)

linux: any $(LIN_BINS)

atch: dtch
	ln -f dtch atch

gfxx-cocoa: gfxx.o gfcocoa.o
	$(CC) $(LDFLAGS) gfxx.o gfcocoa.o $(LDLIBS) -framework Cocoa -o $@

gfxx-fb: gfxx.o gfb.o
	$(CC) $(LDFLAGS) gfxx.o gfb.o $(LDLIBS) -o $@

gfxx:
	[ -f gfxx-cocoa ] && ln -s gfxx-cocoa gfxx || true
	[ -f gfxx-fb ] && ln -s gfxx-fb gfxx || true

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
	echo '*.o' $(ALL_BINS) | tr ' ' '\n' > .gitignore

.PHONY: bsd linux any setuid clean link unlink
