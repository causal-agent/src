PREFIX = ~/.local
MANDIR = ${PREFIX}/share/man

CFLAGS += -Wall -Wextra -Wpedantic -Wno-gnu-case-range
LDLIBS = -lm -lutil -lz

CURL_PREFIX = /usr/local
CFLAGS_curl = ${CFLAGS} -I${CURL_PREFIX}/include
LDFLAGS_curl = ${LDFLAGS} -L${CURL_PREFIX}/lib
LDLIBS_curl = ${LDLIBS} -lcurl

LIBRESSL_PREFIX = /usr/local
CFLAGS_tls = ${CFLAGS} -I${LIBRESSL_PREFIX}/include
LDFLAGS_tls = ${LDFLAGS} -L${LIBRESSL_PREFIX}/lib
LDLIBS_tls = ${LDLIBS} -ltls

-include config.mk

BINS_ANY += aes
BINS_ANY += beef
BINS_ANY += bit
BINS_ANY += dtch
BINS_ANY += edit
BINS_ANY += glitch
BINS_ANY += hi
BINS_ANY += hnel
BINS_ANY += modem
BINS_ANY += order
BINS_ANY += pbd
BINS_ANY += pngo
BINS_ANY += psf2png
BINS_ANY += ptee
BINS_ANY += scheme
BINS_ANY += setopt
BINS_ANY += shotty
BINS_ANY += ttpre
BINS_ANY += up
BINS_ANY += when
BINS_ANY += xx

LINKS_ANY += open
LINKS_ANY += pbcopy
LINKS_ANY += pbpaste

BINS_BSD += ever

BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed

BINS_CURL += title

BINS_TLS += imbox
BINS_TLS += relay

BINS = ${BINS_ANY} ${BINS_BSD} ${BINS_LINUX} ${BINS_CURL} ${BINS_TLS}
MANS = ${BINS:%=man1/%.1}
LINKS = ${LINKS_ANY}

# Groups

any: meta ${BINS_ANY} ${LINKS_ANY}

bsd: meta ${BINS_BSD}

linux: meta ${BINS_LINUX}

curl: meta ${BINS_CURL}

tls: meta ${BINS_TLS}

meta: tags .gitignore

# Binaries

hi: hi.c
	${CC} ${CFLAGS} ${LDFLAGS} hi.c ${LDLIBS} -o $@
	./hi -c

open pbcopy pbpaste: pbd
	ln -f pbd $@

imbox: imbox.c
	${CC} ${CFLAGS_tls} ${LDFLAGS_tls} $@.c ${LDLIBS_tls} -o $@

relay: relay.c
	${CC} ${CFLAGS_tls} ${LDFLAGS_tls} $@.c ${LDLIBS_tls} -o $@

title: title.c
	${CC} ${CFLAGS_curl} ${LDFLAGS_curl} $@.c ${LDLIBS_curl} -o $@

# Headers

fbatt.o fbclock.o: scheme.h

psf2png.o scheme.o: png.h

scheme.h: scheme
	./scheme -c > scheme.h

.o:
	${CC} ${LDFLAGS} $< ${LDLIBS} -o $@

# HTML

HTMLS = index.html ${BINS:%=%.html} png.html
WEBROOT = /usr/local/www/causal.agency

html: ${HTMLS}
	@true

${HTMLS}: html.sh hi ttpre

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

# Meta

scheme.png: scheme
	./scheme -gt > scheme.png

tags: *.h *.c
	ctags -w *.h *.c

IGNORE = '*.o' '*.html' scheme.h scheme.png tags ${BINS} ${LINKS}

.gitignore: Makefile
	echo config.mk ${IGNORE} | tr ' ' '\n' | sort > .gitignore

clean:
	rm -f ${IGNORE}

# Install

link:
	install -d ${PREFIX}/bin ${MANDIR}/man1
	ln -fs ${BINS:%=${PWD}/%} ${LINKS:%=${PWD}/%} ${PREFIX}/bin
	ln -fs ${MANS:%=${PWD}/%} ${MANDIR}/man1

unlink:
	rm -f ${BINS:%=${PREFIX}/bin/%} ${LINKS:%=${PREFIX}/bin/%}
	rm -f ${MANS:%=${MANDIR}/%}

install-html: ${HTMLS}
	install -d ${WEBROOT}/bin
	install -C -m 644 ${HTMLS} ${WEBROOT}/bin

setuid: bri
	chown root bri
	chmod u+s bri
