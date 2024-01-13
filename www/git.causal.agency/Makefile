PREFIX = /var/www
CONFDIR = ${PREFIX}/conf
DATADIR = ${PREFIX}/cgit
BINDIR = ${PREFIX}/bin
WEBROOT = ${PREIFX}/git.causal.agency

CFLAGS += -Wall -Wextra
LDFLAGS = -static -pie

BINS += about-filter
BINS += ctags
BINS += email-filter
BINS += gzip
BINS += hilex
BINS += htagml
BINS += mandoc
BINS += mtags
BINS += owner-filter
BINS += source-filter

HTMLS = index.html

all: ${BINS} ${HTMLS}

compress ctags mandoc:
	${MAKE} -C /usr/src/usr.bin/$@ LDFLAGS='${LDFLAGS}'
	mv /usr/src/usr.bin/$@/$@ $@
	${MAKE} -C /usr/src/usr.bin/$@ clean

gzip: compress
	ln -f compress $@

hilex htagml mtags:
	rm -f ../../bin/$@
	${MAKE} -C ../../bin $@ LDFLAGS='${LDFLAGS}'
	mv ../../bin/$@ $@

about-filter email-filter owner-filter source-filter: filter
	ln -f filter $@

index.html: index.7
	mandoc -Thtml -Ostyle=https://causal.agency/style.css index.7 >index.html

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${CONFDIR}
	install -m 644 custom.css ${DATADIR}
	install -d -o www -g daemon ${PREFIX}/cache/cgit
	install -d -m 1700 -o www -g daemon ${PREFIX}/tmp
	install -s ${BINS} ${BINDIR}
	install -m 644 ${HTMLS} ${WEBROOT}

clean:
	rm -f compress filter ${BINS} ${HTMLS}
