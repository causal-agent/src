PREFIX ?= ~/.local
MANDIR ?= ${PREFIX}/share/man

MODULE = ftp.rfc-editor.org::rfcs-text-only
RFCS = ${MODULE}/rfc-index.txt ${MODULE}/'rfc[1-9]*.txt'

all: rfc rfctags

.SUFFIXES: .in .pl

.in:
	sed 's|%%PREFIX%%|${PREFIX}|g' $< > $@
	chmod a+x $@

.pl:
	cp -f $< $@
	chmod a+x $@

clean:
	rm -f rfc rfctags

install: rfc rfctags rfc.1
	install -d ${PREFIX}/bin ${MANDIR}/man1
	install rfc rfctags ${PREFIX}/bin
	install -m 644 rfc.1 ${MANDIR}/man1
	ln -fs rfc.1 ${MANDIR}/man1/rfctags.1

sync:
	install -d ${PREFIX}/share
	rsync -ptz ${RFCS} ${PREFIX}/share/rfc

compress:
	find ${PREFIX}/share/rfc -name '*.txt' | xargs gzip -9f

uninstall:
	rm -f ${PREFIX}/bin/rfc ${PREFIX}/bin/rfctags
	rm -f ${MANDIR}/man1/rfc.1 ${MANDIR}/man1/rfctags.1
	rm -fr ${PREFIX}/share/rfc
