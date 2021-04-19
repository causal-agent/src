WEBROOT ?= /usr/local/www/causal.agency

HTMLS = index.html png.html
HTMLS += ${BINS:=.html}
HTMLS += ${BSD:=.html}
HTMLS += ${GAMES:=.html}
HTMLS += ${LINUX:=.html}
HTMLS += ${TLS:=.html}

html: ${HTMLS}
	@true

install-html: ${HTMLS}
	install -d ${WEBROOT}/bin
	install -C -m 644 ${HTMLS} ${WEBROOT}/bin

${HTMLS}: html.sh scheme hilex htagml htmltags

htmltags: *.[chly] mtags Makefile html.mk *.sh
	rm -f $@
	for f in *.[chly]; do ctags -aw -f $@ $$f; done
	./mtags -a -f $@ Makefile html.mk *.sh

index.html: README.7 Makefile html.mk html.sh
	sh html.sh README.7 Makefile html.mk html.sh > $@

.SUFFIXES: .html

.c.html:
	sh html.sh man1/${<:.c=.1} $< > $@

.h.html:
	sh html.sh man3/${<:.h=.3} $< > $@

.y.html:
	sh html.sh man1/${<:.y=.1} $< > $@

.sh.html:
	sh html.sh man1/${<:.sh=.1} $< > $@

.pl.html:
	sh html.sh man1/${<:.pl=.1} $< > $@

freecell.html: freecell.c man6/freecell.6
	sh html.sh man6/freecell.6 freecell.c > $@
