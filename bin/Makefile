PINS = atch bri dtch hnel pbcopy pbd pbpaste wake xx
BINS = $(PINS) jrp klon typo watch
CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil

all: $(BINS) .gitignore

portable: $(PINS) .gitignore

atch: dtch
	ln -f dtch atch

pbcopy pbpaste: pbd
	ln -f pbd $@

clean:
	rm -f $(BINS)

link:
	ln -s -f $(BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(BINS:%=~/.bin/%)

.gitignore: Makefile
	echo $(BINS) | tr ' ' '\n' > .gitignore

.PHONY: all clean link unlink
