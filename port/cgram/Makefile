PREFIX = ~/.local
MANDIR = ${PREFIX}/share/man

LDLIBS = -lcurses

cgram:

clean:
	rm -f cgram

install: cgram cgram.6
	install -d ${PREFIX}/bin ${MANDIR}/man6
	install cgram ${PREFIX}/bin
	install -m 644 cgram.6 ${MANDIR}/man6

uninstall:
	rm -f ${PREFIX}/bin/cgram ${MANDIR}/man6/cgram.6
