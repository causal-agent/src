PREFIX = ~/.local
MANDIR = ${PREFIX}/share/man

LIBS_PREFIX = /usr/local
CFLAGS += -I${LIBS_PREFIX}/include
LDFLAGS += -L${LIBS_PREFIX}/lib

CFLAGS += -Wall -Wextra -Wpedantic -Wno-gnu-case-range
LDLIBS.dtch = -lutil
LDLIBS.glitch = -lz
LDLIBS.hnel = -lutil
LDLIBS.modem = -lutil
LDLIBS.pngo = -lz
LDLIBS.ptee = -lutil
LDLIBS.relay = -ltls
LDLIBS.scheme = -lm
LDLIBS.title = -lcurl

-include config.mk

BINS += beef
BINS += bit
BINS += c
BINS += dtch
BINS += edit
BINS += glitch
BINS += hi
BINS += hnel
BINS += modem
BINS += order
BINS += pbd
BINS += pngo
BINS += psf2png
BINS += ptee
BINS += scheme
BINS += shotty
BINS += title
BINS += ttpre
BINS += up
BINS += when
BINS += xx

LINKS += open
LINKS += pbcopy
LINKS += pbpaste

BINS_BSD += ever
BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed
BINS_TLS += relay

BINS_ALL = ${BINS} ${BINS_BSD} ${BINS_LINUX} ${BINS_TLS}
MANS_ALL = ${BINS_ALL:%=man1/%.1}

any: meta ${BINS} ${LINKS}

bsd: meta ${BINS_BSD}

linux: meta ${BINS_LINUX}

tls: meta ${BINS_TLS}

meta: .gitignore tags

.c:
	${CC} ${CFLAGS} ${LDFLAGS} $< ${LDLIBS.$@} -o $@

.o:
	${CC} ${LDFLAGS} $< ${LDLIBS.$@} -o $@

hi: hi.c
	${CC} ${CFLAGS} ${LDFLAGS} hi.c ${LDLIBS.$@} -o $@
	./hi -c

open pbcopy pbpaste: pbd
	ln -f pbd $@

fbatt.o fbclock.o: scheme.h

psf2png.o scheme.o: png.h

scheme.h: scheme
	./scheme -c > scheme.h

tags: *.h *.c
	ctags -w *.h *.c

IGNORE = *.o *.html ${BINS_ALL} ${LINKS} scheme.h tags

.gitignore: Makefile
	echo config.mk '${IGNORE}' | tr ' ' '\n' | sort > .gitignore

clean:
	rm -f ${IGNORE}

setuid: bri
	chown root bri
	chmod u+s bri

link:
	install -d ${PREFIX}/bin ${MANDIR}/man1
	ln -fs ${BINS_ALL:%=${PWD}/%} ${PREFIX}/bin
	ln -fs ${MANS_ALL:%=${PWD}/%} ${MANDIR}/man1
	ln -fs ${LINKS:%=${PWD}/%} ${PREFIX}/bin

unlink:
	rm -f ${BINS_ALL:%=${PREFIX}/bin/%}
	rm -f ${MANS_ALL:%=${MANDIR}/%}
	rm -f ${LINKS:%=${PREFIX}/bin/%}

HTMLS = index.html ${BINS_ALL:=.html} png.html
WEBROOT = /usr/local/www/causal.agency

html: ${HTMLS}
	@true

${HTMLS}: html.sh scheme hi ttpre

.SUFFIXES: .html

.c.html:
	sh html.sh $< man1/${<:.c=.1} > $@

.h.html:
	sh html.sh $< man3/${<:.h=.3} > $@

.sh.html:
	sh html.sh $< man1/${<:.sh=.1} > $@

.y.html:
	sh html.sh $< man1/${<:.y=.1} > $@

index.html: Makefile README.7
	sh html.sh Makefile README.7 \
		| sed -E 's,([a-z0-9-]+)[(][1-9][)],<a href="\1.html">&</a>,' \
		> index.html

install-html: ${HTMLS}
	install -d ${WEBROOT}/bin
	install -C -m 644 ${HTMLS} ${WEBROOT}/bin
