PREFIX = ~/.local

CFLAGS = -Dmk_wcwidth=wcwidth -Dmk_wcswidth=wcswidth

OBJS = wcwidth.o
DYLIB = libwcwidth.dylib

${DYLIB}: ${OBJS}
	${CC} -dynamiclib ${LDFLAGS} ${OBJS} -o $@

install: ${DYLIB}
	install -d ${PREFIX}/lib
	install -m 644 ${DYLIB} ${PREFIX}/lib

uninstall:
	rm -f ${PREFIX}/lib/${DYLIB}
