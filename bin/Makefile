ANY_BINS = atch dtch hnel pbcopy pbd pbpaste wake xx
BSD_BINS = jrp klon typo watch
LIN_BINS = bri fbclock gfxx
ALL_BINS = $(ANY_BINS) $(BSD_BINS) $(LIN_BINS)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcurses -ledit -lutil -lz

any: $(ANY_BINS) .gitignore

bsd: $(ANY_BINS) $(BSD_BINS) .gitignore

linux: $(ANY_BINS) $(LIN_BINS) .gitignore

atch: dtch
	ln -f dtch atch

pbcopy pbpaste: pbd
	ln -f pbd $@

setuid: bri
	chown root bri
	chmod u+s bri

clean:
	rm -f $(ALL_BINS)

link:
	ln -s -f $(ALL_BINS:%=$(PWD)/%) ~/.bin

unlink:
	rm -f $(ALL_BINS:%=~/.bin/%)

.gitignore: Makefile
	echo $(ALL_BINS) | tr ' ' '\n' > .gitignore

.PHONY: bsd linux any setuid clean link unlink
