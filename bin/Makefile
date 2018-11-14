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
BINS += ttpre
BINS += wake
BINS += xx

BINS_BSD += watch

BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed

BINS_ALL = $(BINS) $(BINS_BSD) $(BINS_LINUX)
MAN1_ALL = $(BINS_ALL:%=man/%.1)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -lm -lutil -lz

any: .gitignore tags $(BINS)

bsd: any $(BINS_BSD)

linux: any $(BINS_LINUX)

.gitignore: Makefile
	echo '*.o' tags scheme.h $(BINS_ALL) scheme.png \
		| tr ' ' '\n' \
		> .gitignore

tags: *.c
	ctags -w *.c

atch: dtch
	ln -f dtch atch

fbatt.o fbclock.o: scheme.h

scheme.h: scheme
	./scheme -c > scheme.h

open pbcopy pbpaste: pbd
	ln -f pbd $@

psf2png.o scheme.o: png.h

scheme.png: scheme
	./scheme -t -g > scheme.png

clean:
	rm -f *.o tags scheme.h $(BINS_ALL) scheme.png

README: man/bin.7
	mandoc man/bin.7 | col -b -x > README

setuid: bri
	chown root bri
	chmod u+s bri

link:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	ln -s -f $(BINS_ALL:%=$(PWD)/%) $(PREFIX)/bin
	ln -s -f $(MAN1_ALL:%=$(PWD)/%) $(PREFIX)/share/man/man1

unlink:
	rm -f $(BINS_ALL:%=$(PREFIX)/bin/%)
	rm -f $(MAN1_ALL:%=$(PREFIX)/share/man/man1/%)
