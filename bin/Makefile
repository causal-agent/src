PREFIX = ~/.local

BINS += atch
BINS += dtch
BINS += glitch
BINS += hnel
BINS += modem
BINS += open
BINS += pbcopy
BINS += pbd
BINS += pbpaste
BINS += pngo
BINS += psf2png
BINS += scheme
BINS += wake
BINS += xx

BINS_BSD = watch
BINS_LINUX = bri fbatt fbclock psfed
BINS_ALL = $(BINS) $(BINS_BSD) $(BINS_LINUX)

GAMES_BSD = klon
GAMES_ALL = $(GAMES_BSD)

MAN1 = $(BINS_ALL:%=man/%.1)
MAN6 = $(GAMES_ALL:%=man/%.6)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -lm -lutil -lz

any: .gitignore tags $(BINS)

bsd: any $(BINS_BSD) $(GAMES_BSD)

linux: any $(BINS_LINUX)

tags: *.c
	ctags -w *.c

scheme.h: scheme
	./scheme -c > scheme.h

fbatt.o fbclock.o: scheme.h

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
	rm -f tags scheme.h *.o $(BINS_ALL) $(GAMES_ALL) scheme.png

README: man/bin.7
	mandoc man/bin.7 | col -b -x > README

.gitignore: Makefile
	echo tags scheme.h *.o $(BINS_ALL) $(GAMES_ALL) scheme.png \
		| tr ' ' '\n' \
		> .gitignore

link:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1 $(PREFIX)/share/man/man6
	ln -s -f $(BINS_ALL:%=$(PWD)/%) $(PREFIX)/bin
	ln -s -f $(GAMES_ALL:%=$(PWD)/%) $(PREFIX)/bin
	ln -s -f $(MAN1:%=$(PWD)/%) $(PREFIX)/share/man/man1
	ln -s -f $(MAN6:%=$(PWD)/%) $(PREFIX)/share/man/man6

unlink:
	rm -f $(BINS_ALL:%=$(PREFIX)/bin/%)
	rm -f $(GAMES_ALL:%=$(PREFIX)/bin/%)
	rm -f $(MAN1:%=$(PREFIX)/share/man/man1/%)
	rm -f $(MAN6:%=$(PREFIX)/share/man/man6/%)
