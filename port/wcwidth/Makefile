PREFIX ?= ~/.local

OBJS = wcwidth.o wcswidth.o

all: libwcwidth.dylib wcfix

libwcwidth.dylib: ${OBJS}
	${CC} -dynamiclib ${LDFLAGS} ${OBJS} -o $@

wcwidth.o: nonspacing.h wide.h

.SUFFIXES: .in

.in:
	sed 's|%%PREFIX%%|${PREFIX}|g' $< > $@
	chmod a+x $@

clean:
	rm -f libwcwidth.dylib wcfix ${OBJS}

install: libwcwidth.dylib wcfix
	install -d ${PREFIX}/lib ${PREFIX}/bin
	install -m 644 libwcwidth.dylib ${PREFIX}/lib
	install wcfix ${PREFIX}/bin

uninstall:
	rm -f ${PREFIX}/lib/libwcwidth.dylib ${PREFIX}/bin/wcfix
