CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lcursesw

OBJS += buffer.o
OBJS += edi.o
OBJS += file.o
OBJS += log.o
OBJS += table.o

TESTS += buffer.t
TESTS += table.t

all: tags edi test

tags: *.h *.c
	ctags -w *.h *.c

edi: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(OBJS): edi.h

test: $(TESTS)
	set -e; $(TESTS:%=./%;)

.SUFFIXES: .t

.c.t:
	$(CC) $(CFLAGS) -DTEST $(LDFLAGS) $< $(LDLIBS) -o $@

clean:
	rm -f tags edi $(OBJS) $(TESTS)
