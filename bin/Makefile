BINS = atch bri dtch hnel ish jrp pbcopy pbd pbpaste typo wake watch xx
CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil

all: $(BINS) .gitignore

atch: dtch
	ln -f dtch atch

pbcopy pbpaste: pbd
	ln -f pbd $@

clean:
	rm -f $(BINS)

link: $(BINS)
	ln -s -f $(BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(BINS:%=~/.bin/%)

.gitignore: Makefile
	echo $(BINS) | tr ' ' '\n' > .gitignore

.PHONY: all clean link unlink
