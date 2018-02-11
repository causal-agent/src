ANY_BINS = atch dtch gfxx hnel pbcopy pbd pbpaste wake xx
BSD_BINS = jrp klon typo watch
LIN_BINS = bri fbatt fbclock
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(LIN_BINS)
GFX ?= none

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -ledit -lncurses -lutil -lz
LDLIBS_cocoa = -framework Cocoa
LDLIBS_x11 = -lX11

any: .gitignore tags $(ANY_BINS)

bsd: any $(BSD_BINS)

linux: any $(LIN_BINS)

atch: dtch
	ln -f dtch atch

gfxx: gfxx.o gfx/$(GFX).o
	$(CC) $(LDFLAGS) gfxx.o gfx/$(GFX).o $(LDLIBS) $(LDLIBS_$(GFX)) -o $@

pbcopy pbpaste: pbd
	ln -f pbd $@

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f tags *.o gfx/*.o $(ALL_BINS)

link:
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(ALL_BINS:%=~/.bin/%)

.gitignore: Makefile
	echo '*.o' tags $(ALL_BINS) | tr ' ' '\n' > .gitignore

tags: *.c
	ctags -w *.c

.PHONY: any bsd linux setuid clean link unlink
