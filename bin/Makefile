PREFIX = ~/.local
MANDIR = $(PREFIX)/share/man
GFX = cocoa

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lm -lutil -lz
LDLIBS_cocoa = $(LDLIBS) -framework Cocoa
LDLIBS_fb = $(LDLIBS)
LDLIBS_x11 = $(LDLIBS) -lX11

BINS += aes
BINS += dtch
BINS += glitch
BINS += hi
BINS += hnel
BINS += modem
BINS += pbd
BINS += pngo
BINS += psf2png
BINS += scheme
BINS += ttpre
BINS += up
BINS += wake
BINS += xx

LINKS += atch
LINKS += open
LINKS += pbcopy
LINKS += pbpaste

BINS_BSD += beef
BINS_BSD += wat

BINS_LINUX += bri
BINS_LINUX += fbatt
BINS_LINUX += fbclock
BINS_LINUX += psfed

BINS_GFX += brot
BINS_GFX += gfxx

BINS_ALL = $(BINS) $(BINS_BSD) $(BINS_LINUX) $(BINS_GFX)
MAN1_ALL = $(BINS_ALL:%=man1/%.1)

any: .gitignore tags $(BINS) $(LINKS)

bsd: any $(BINS_BSD)

linux: any $(BINS_LINUX)

gfx: any $(BINS_GFX)

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

atch: dtch
	ln -f dtch atch

open pbcopy pbpaste: pbd
	ln -f pbd $@

.gitignore: Makefile
	echo $(BINS_ALL) $(LINKS) tags scheme.h scheme.png '*.o' '*.html' \
		| tr ' ' '\n' | sort > .gitignore

tags: *.h *.c
	ctags -w *.h *.c

scheme.png: scheme
	./scheme -gt > scheme.png

HTMLS = $(BINS_ALL:%=%.html)
WEBROOT = /usr/local/www/causal.agency
GITEA_URL = https://code.causal.agency/june/src/src/branch/master/bin

html: $(HTMLS)

$(HTMLS): ttpre hi

.SUFFIXES: .html

.c.html:
	@./hi -f html -o document,anchor,tab=4 -m $< /dev/null > $@
	@echo '<code><a href="$(GITEA_URL)/$<">$< in git</a></code>' >> $@
	man -P cat ./man1/$(<:%.c=%.1) | ./ttpre >> $@
	./hi -n -f html -o anchor $< >> $@

.sh.html:
	@./hi -f html -o document,anchor,tab=4 -m $< /dev/null > $@
	@echo '<code><a href="$(GITEA_URL)/$<">$< in git</a></code>' >> $@
	man -P cat ./man1/$(<:%.sh=%.1) | ./ttpre >> $@
	./hi -n -f html -o anchor $< >> $@

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
