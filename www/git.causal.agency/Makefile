PREFIX = /var/www
CONFDIR = ${PREFIX}/conf
DATADIR = ${PREFIX}/cgit
BINDIR = ${PREFIX}/bin

BINS += about-filter
BINS += cat
BINS += compress
BINS += ctags
BINS += email-filter
BINS += gzip
BINS += hilex
BINS += htagml
BINS += mandoc
BINS += mktemp
BINS += mtags
BINS += owner-filter
BINS += rm
BINS += sh
BINS += source-filter

all: ${BINS}

cat rm sh:
	cp /bin/$@ $@

compress ctags mandoc mktemp:
	${MAKE} -C /usr/src/usr.bin/$@ LDFLAGS='-static -pie'
	mv /usr/src/usr.bin/$@/$@ $@
	${MAKE} -C /usr/src/usr.bin/$@ clean

gzip: compress
	cp compress $@

hilex htagml mtags:
	rm -f ../../bin/$@
	${MAKE} -C ../../bin $@ LDFLAGS='-static -pie'
	mv ../../bin/$@ $@

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${CONFDIR}
	install -m 644 custom.css ${DATADIR}
	install -d -o www -g daemon ${PREFIX}/cache/cgit
	install -d -m 1700 -o www -g daemon ${PREFIX}/tmp
	install ${BINS} ${BINDIR}

clean:
	rm -f ${BINS}
