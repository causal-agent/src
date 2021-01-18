PREFIX ?= ~/.local
MANDIR ?= ${PREFIX}/share/man

BINS = rfc rfctags
MODULE = ftp.rfc-editor.org::rfcs-text-only
RFCS = ${MODULE}/rfc-index.txt ${MODULE}/'rfc[1-9]*.txt'

all: ${BINS}

.SUFFIXES: .in .pl

.in:
	sed 's|%%PREFIX%%|${PREFIX}|g' $< > $@
	chmod a+x $@

.pl:
	cp -f $< $@
	chmod a+x $@

clean:
	rm -f ${BINS}

install: ${BINS} rfc.1
	install -d ${PREFIX}/bin ${MANDIR}/man1 ${PREFIX}/share
	install ${BINS} ${PREFIX}/bin
	install -m 644 rfc.1 ${MANDIR}/man1
	ln -fs rfc.1 ${MANDIR}/man1/rfctags.1
	rsync -ptz ${RFCS} ${PREFIX}/share/rfc

install-nvim: install rfc.vim
	install -d ${PREFIX}/share/nvim/site/plugin
	install -m 644 rfc.vim ${PREFIX}/share/nvim/site/plugin
	(cd ${PREFIX}/share/rfc \
		&& printf '!_TAG_FILE_SORTED\t2\ttags\n' \
		&& ${PREFIX}/bin/rfctags *.txt) | sort -f > ${PREFIX}/share/rfc/tags

uninstall:
	rm -f ${BINS:%=${PREFIX}/bin/%} ${BINS:%=${MANDIR}/man1/%.1}
	rm -f ${PREFIX}/share/nvim/site/plugin/rfc.vim
	rm -fr ${PREFIX}/share/rfc
