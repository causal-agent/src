PREFIX = ~/.local

BIN += atch
BIN += dtch
BIN += glitch
BIN += hnel
BIN += modem
BIN += open
BIN += pbcopy
BIN += pbd
BIN += pbpaste
BIN += pngo
BIN += psf2png
BIN += scheme
BIN += ttpre
BIN += wake
BIN += xx

BIN_BSD += wat

BIN_LINUX += bri
BIN_LINUX += fbatt
BIN_LINUX += fbclock
BIN_LINUX += psfed

BIN_ALL = $(BIN) $(BIN_BSD) $(BIN_LINUX)
MAN_ALL = $(BIN_ALL:%=man/%.1)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lm -lutil -lz

any: .gitignore tags $(BIN)

bsd: any $(BIN_BSD)

linux: any $(BIN_LINUX)

scheme.h: scheme
	./scheme -c > scheme.h

fbatt.o fbclock.o: scheme.h

psf2png.o scheme.o: png.h

atch: dtch
	ln -f dtch atch

open pbcopy pbpaste: pbd
	ln -f pbd $@

scheme.png: scheme
	./scheme -t -g > scheme.png

tags: *.h *.c
	ctags -w *.h *.c

.gitignore: Makefile
	echo '*.o' scheme.h scheme.png tags $(BIN_ALL) \
		| tr ' ' '\n' \
		> .gitignore

clean:
	rm -f *.o scheme.h scheme.png tags $(BIN_ALL)

README: man/bin.7
	mandoc man/bin.7 | col -b -x > README

setuid: bri
	chown root bri
	chmod u+s bri

link:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	ln -s -f $(BIN_ALL:%=$(PWD)/%) $(PREFIX)/bin
	ln -s -f $(MAN_ALL:%=$(PWD)/%) $(PREFIX)/share/man/man1

unlink:
	rm -f $(BIN_ALL:%=$(PREFIX)/bin/%)
	rm -f $(MAN_ALL:man/%=$(PREFIX)/share/man/man1/%)
