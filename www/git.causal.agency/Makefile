ETC = /usr/local/etc
WWW = /usr/local/www/cgit
LIBEXEC = /usr/local/libexec

BIN = ../../bin
BINS = about-filter source-filter hilex htagml

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${ETC}
	install -m 644 custom.css ${WWW}
	install ${BINS} ${LIBEXEC}

hilex htagml::
	${MAKE} -C ${BIN} $@
	ln -f ${BIN}/$@ $@

clean:
	rm -f ${BINS}
