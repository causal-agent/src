CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

OBJS += c.o
OBJS += hilex.o
OBJS += make.o
OBJS += mdoc.o
OBJS += text.o

hilex: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} ${LDLIBS} -o $@

${OBJS}: hilex.h

clean:
	rm -f hilex ${OBJS}
