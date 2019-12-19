ETC = /usr/local/etc
WWW = /usr/local/www/cgit
LIBEXEC = /usr/local/libexec

BIN = ../../bin
BINS = source-filter ttpre hi

install: cgitrc custom.css ${BINS}
	install -m 644 cgitrc ${ETC}
	install -m 644 custom.css ${WWW}
	install ${BINS} ${LIBEXEC}

ttpre hi:
	${MAKE} -C ${BIN} $@

ttpre: ${BIN}/ttpre.c

hi: ${BIN}/hi.c

clean:
	rm -f ${BINS}
