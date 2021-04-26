PREFIX ?= ~/.local
MANDIR ?= ${PREFIX}/share/man

LIBS_PREFIX ?= /usr/local
CFLAGS += -I${LIBS_PREFIX}/include
LDFLAGS += -L${LIBS_PREFIX}/lib

CFLAGS += -Wall -Wextra -Wpedantic -Wno-gnu-case-range

BINS += beef
BINS += bibsort
BINS += bit
BINS += c
BINS += dtch
BINS += glitch
BINS += hilex
BINS += htagml
BINS += modem
BINS += mtags
BINS += nudge
BINS += order
BINS += pbd
BINS += pngo
BINS += psf2png
BINS += ptee
BINS += scheme
BINS += shotty
BINS += sup
BINS += title
BINS += up
BINS += when
BINS += xx

BSD += ever

GAMES += freecell

LINUX += bri
LINUX += fbatt
LINUX += fbclock
LINUX += psfed

TLS += relay
TLS += typer

MANS = ${BINS:%=man1/%.1}
MANS.BSD = ${BSD:%=man1/%.1}
MANS.GAMES = ${GAMES:%=man6/%.6}
MANS.LINUX = ${LINUX:%=man1/%.1}
MANS.TLS = ${TLS:%=man1/%.1}

LDLIBS.dtch = -lutil
LDLIBS.fbclock = -lz
LDLIBS.freecell = -lcurses
LDLIBS.glitch = -lz
LDLIBS.modem = -lutil
LDLIBS.pngo = -lz
LDLIBS.ptee = -lutil
LDLIBS.relay = -ltls
LDLIBS.scheme = -lm
LDLIBS.title = -lcurl
LDLIBS.typer = -ltls

ALL ?= meta any

-include config.mk

all: ${ALL}

meta: .gitignore tags

any: ${BINS}

bsd: ${BSD}

games: ${GAMES}

linux: ${LINUX}

tls: ${TLS}

IGNORE = *.o *.html
IGNORE += ${BINS} ${BSD} ${GAMES} ${LINUX} ${TLS}
IGNORE += scheme.h tags htmltags

.gitignore: Makefile
	echo config.mk '${IGNORE}' | tr ' ' '\n' | sort > $@

tags: *.[chly]
	ctags -w *.[chly]

clean:
	rm -f ${IGNORE}

install: ${ALL:%=install-%}

install-meta:
	install -d ${PREFIX}/bin ${MANDIR}/man1

install-any: install-meta ${BINS} ${MANS}
	install ${BINS} ${PREFIX}/bin
	install -m 644 ${MANS} ${MANDIR}/man1

install-bsd: install-meta ${BSD} ${MANS.BSD}
	install ${BSD} ${PREFIX}/bin
	install -m 644 ${MANS.BSD} ${MANDIR}/man1

install-games: install-meta ${GAMES} ${MANS.GAMES}
	install ${GAMES} ${PREFIX}/bin
	install -m 644 ${MANS.GAMES} ${MANDIR}/man6

install-linux: install-meta ${LINUX} ${MANS.LINUX}
	install ${LINUX} ${PREFIX}/bin
	install -m 644 ${MANS.LINUX} ${MANDIR}/man1

install-tls: install-meta ${TLS} ${MANS.TLS}
	install ${TLS} ${PREFIX}/bin
	install -m 644 ${MANS.TLS} ${MANDIR}/man1

uninstall:
	rm -f ${BINS:%=${PREFIX}/bin/%} ${MANS:%=${MANDIR}/%}
	rm -f ${BSD:%=${PREFIX}/bin/%} ${MANS.BSD:%=${MANDIR}/%}
	rm -f ${GAMES:%=${PREFIX}/bin/%} ${MANS.GAMES:%=${MANDIR}/%}
	rm -f ${LINUX:%=${PREFIX}/bin/%} ${MANS.LINUX:%=${MANDIR}/%}
	rm -f ${TLS:%=${PREFIX}/bin/%} ${MANS.TLS:%=${MANDIR}/%}

.SUFFIXES: .pl

.c:
	${CC} ${CFLAGS} ${LDFLAGS} $< ${LDLIBS.$@} -o $@

.o:
	${CC} ${LDFLAGS} $< ${LDLIBS.$@} -o $@

.pl:
	cp -f $< $@
	chmod a+x $@

OBJS.hilex = c11.o hilex.o make.o mdoc.o sh.o

hilex: ${OBJS.hilex}
	${CC} ${LDFLAGS} ${OBJS.$@} ${LDLIBS.$@} -o $@

${OBJS.hilex}: hilex.h

fbatt.o fbclock.o: scheme.h

psf2png.o scheme.o: png.h

scheme.h: scheme
	./scheme -c > $@

include html.mk
