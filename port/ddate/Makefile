PREFIX ?= ~/.local
MANDIR ?= ${PREFIX}/share/man

ddate:

clean:
	rm -f ddate

install: ddate ddate.1
	install -d ${PREFIX}/bin ${MANDIR}/man1
	install ddate ${PREFIX}/bin
	install -m 644 ddate.1 ${MANDIR}/man1

uninstall:
	rm -f ${PREFIX}/bin/ddate ${MANDIR}/man1/ddate.1
