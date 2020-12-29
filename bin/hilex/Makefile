CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

OBJS += ansi.o
OBJS += c.o
OBJS += hilex.o
OBJS += irc.o
OBJS += mdoc.o
OBJS += text.o

hilex: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} ${LDLIBS} -o $@

${OBJS}: hilex.h

clean:
	rm -f hilex ${OBJS}
