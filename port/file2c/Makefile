PREFIX = ~/.local
MANDIR = ${PREFIX}/share/man

file2c:

clean:
	rm -f file2c

install: file2c file2c.1
	install -d ${PREFIX}/bin ${MANDIR}/man1
	install file2c ${PREFIX}/bin
	install -m 644 file2c.1 ${MANDIR}/man1

uninstall:
	rm -f ${PREFIX}/bin/file2c ${MANDIR}/man1/file2c.1
