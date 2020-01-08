ETC = /usr/local/etc
WWW = /usr/local/www/cgit
LIBEXEC = /usr/local/libexec

BIN = ../../bin
BINS = about-filter source-filter hi

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${ETC}
	install -m 644 custom.css ${WWW}
	install ${BINS} ${LIBEXEC}

hi: ${BIN}/hi.c
	${MAKE} -C ${BIN} $@
	cp ${BIN}/$@ $@

clean:
	rm -f ${BINS}
