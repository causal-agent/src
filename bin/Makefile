PREFIX = ~/.local
MANDIR = $(PREFIX)/share/man

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

BINS_BSD += wat

BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed

BINS_ALL = $(BINS) $(BINS_BSD) $(BINS_LINUX)
MAN1_ALL = $(BINS_ALL:%=man1/%.1)

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS = -lm -lutil -lz

any: .gitignore tags $(BINS)

bsd: any $(BINS_BSD)

linux: any $(BINS_LINUX)

.o:
	$(CC) $(LDFLAGS) $< $(LDLIBS) -o $@

scheme.h: scheme
	./scheme -c > scheme.h

fbatt.o fbclock.o: scheme.h

psf2png.o scheme.o: png.h

atch: dtch
	ln -f dtch atch

open pbcopy pbpaste: pbd
	ln -f pbd $@

.gitignore: Makefile
	echo $(BINS_ALL) tags scheme.h scheme.png '*.o' \
		| tr ' ' '\n' | sort > .gitignore

tags: *.h *.c
	ctags -w *.h *.c

scheme.png: scheme
	./scheme -gt > scheme.png

clean:
	rm -f $(BINS_ALL) tags scheme.h scheme.png *.o

README: bin.7
	mandoc bin.7 | col -bx > README

setuid: bri
	chown root bri
	chmod u+s bri

link:
	install -d $(PREFIX)/bin $(MANDIR)/man1
	ln -fs $(BINS_ALL:%=$(PWD)/%) $(PREFIX)/bin
	ln -fs $(MAN1_ALL:%=$(PWD)/%) $(MANDIR)/man1

unlink:
	rm -f $(BINS_ALL:%=$(PREFIX)/bin/%)
	rm -f $(MAN1_ALL:%=$(MANDIR)/%)
