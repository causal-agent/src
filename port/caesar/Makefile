PREFIX = ~/.local
MANDIR = ${PREFIX}/share/man

LDLIBS = -lm

all: caesar rot13

clean:
	rm -f caesar rot13

install: caesar rot13 caesar.6
	install -d ${PREFIX}/bin ${MANDIR}/man6
	install caesar rot13 ${PREFIX}/bin
	install -m 644 caesar.6 ${MANDIR}/man6/caesar.6
	install -m 644 caesar.6 ${MANDIR}/man6/rot13.6

uninstall:
	rm -f ${PREFIX}/bin/caesar ${PREFIX}/bin/rot13
	rm -f ${MANDIR}/man6/caesar.6 ${MANDIR}/man6/rot13.6
