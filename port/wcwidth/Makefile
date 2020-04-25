PREFIX = ~/.local

OBJS = wcwidth.o wcswidth.o
DYLIB = libwcwidth.dylib

${DYLIB}: ${OBJS}
	${CC} -dynamiclib ${LDFLAGS} ${OBJS} -o $@

wcwidth.o: nonspacing.h wide.h

clean:
	rm -f ${DYLIB} ${OBJS}

install: ${DYLIB}
	install -d ${PREFIX}/lib
	install -m 644 ${DYLIB} ${PREFIX}/lib

uninstall:
	rm -f ${PREFIX}/lib/${DYLIB}
