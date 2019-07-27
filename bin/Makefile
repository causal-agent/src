GFX = cocoa
LIBRESSL_PREFIX = /usr/local

PREFIX = ~/.local
MANDIR = $(PREFIX)/share/man

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lm -lutil -lz

CFLAGS_tls = $(CFLAGS) -I$(LIBRESSL_PREFIX)/include
LDFLAGS_tls = $(LDFLAGS) -L$(LIBRESSL_PREFIX)/lib
LDLIBS_tls = $(LDLIBS) -ltls

LDLIBS_cocoa = $(LDLIBS) -framework Cocoa
LDLIBS_fb = $(LDLIBS)
LDLIBS_x11 = $(LDLIBS) -lX11

-include config.mk

BINS += aes
BINS += bit
BINS += dtch
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
BINS += setopt
BINS += shotty
BINS += ttpre
BINS += up
BINS += wake
BINS += when
BINS += xx

LINKS += atch
LINKS += open
LINKS += pbcopy
LINKS += pbpaste

BINS_BSD += beef
BINS_BSD += wat

BINS_GFX += brot
BINS_GFX += gfxx

BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed

BINS_TLS += relay

BINS_ALL = $(BINS) $(BINS_BSD) $(BINS_GFX) $(BINS_LINUX) $(BINS_TLS)
MAN1_ALL = $(BINS_ALL:%=man1/%.1)

any: .gitignore tags $(BINS) $(LINKS)

bsd: $(BINS_BSD)

gfx: $(BINS_GFX)

linux: $(BINS_LINUX)

tls: $(BINS_TLS)

.o:
	$(CC) $(LDFLAGS) $< $(LDLIBS) -o $@

scheme.h: scheme
	./scheme -c > scheme.h

brot.o gfxx.o gfx-cocoa.o gfx-fb.o gfx-x11.o: gfx.h

fbatt.o fbclock.o: scheme.h

gfxx.o psf2png.o scheme.o: png.h

GFX_OBJ = gfx-$(GFX).o

brot: brot.o $(GFX_OBJ)
	$(CC) $(LDFLAGS) $@.o $(GFX_OBJ) $(LDLIBS_$(GFX)) -o $@

gfxx: gfxx.o $(GFX_OBJ)
	$(CC) $(LDFLAGS) $@.o $(GFX_OBJ) $(LDLIBS_$(GFX)) -o $@

hi: hi.c
	$(CC) $(CFLAGS) $(LDFLAGS) hi.c $(LDLIBS) -o $@
	./hi -c

relay: relay.c
	$(CC) $(CFLAGS_tls) $(LDFLAGS_tls) relay.c $(LDLIBS_tls) -o $@

atch: dtch
	ln -f dtch atch

open pbcopy pbpaste: pbd
	ln -f pbd $@

tags: *.h *.c
	ctags -w *.h *.c

scheme.png: scheme
	./scheme -gt > scheme.png

IGNORE = '*.o' '*.html' config.mk tags scheme.h scheme.png $(BINS_ALL) $(LINKS)

.gitignore: Makefile
	echo $(IGNORE) | tr ' ' '\n' | sort > .gitignore

HTMLS = $(BINS_ALL:%=%.html)
HTMLS += index.html
HTMLS += png.html
WEBROOT = /usr/local/www/causal.agency

html: $(HTMLS)
	@true

$(HTMLS): ttpre hi html.sh

.SUFFIXES: .html

.c.html:
	sh html.sh $< man1/$(<:.c=.1) > $@

.y.html:
	sh html.sh $< man1/$(<:.y=.1) > $@

.sh.html:
	sh html.sh $< man1/$(<:.sh=.1) > $@

.h.html:
	sh html.sh $< man3/$(<:.h=.3) > $@

index.html: Makefile bin.7
	sh html.sh Makefile bin.7 \
		| sed -E -e 's#([a-z0-9-]+)[(]1[)]#<a href="\1.html">&</a>#' \
		> index.html

clean:
	rm -f $(BINS_ALL) $(LINKS) tags scheme.h scheme.png *.o *.html

README: bin.7
	mandoc bin.7 | col -bx > README

setuid: bri
	chown root bri
	chmod u+s bri

link:
	install -d $(PREFIX)/bin $(MANDIR)/man1
	ln -fs $(BINS_ALL:%=$(PWD)/%) $(LINKS:%=$(PWD)/%) $(PREFIX)/bin
	ln -fs $(MAN1_ALL:%=$(PWD)/%) $(MANDIR)/man1

unlink:
	rm -f $(BINS_ALL:%=$(PREFIX)/bin/%) $(LINKS:%=$(PREFIX)/bin/%)
	rm -f $(MAN1_ALL:%=$(MANDIR)/%)

install-html: $(HTMLS)
	install -d $(WEBROOT)/bin
	install -C -m 644 $(HTMLS) $(WEBROOT)/bin
