ETC = /usr/local/etc
WWW = /usr/local/www/cgit
LIBEXEC = /usr/local/libexec

BIN = ../../bin
BINS = about-filter source-filter hilex htagml mtags

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${ETC}
	install -m 644 custom.css ${WWW}
	install ${BINS} ${LIBEXEC}

hilex htagml mtags::
	${MAKE} -C ${BIN} $@
	ln -f ${BIN}/$@ $@

clean:
	rm -f ${BINS}
