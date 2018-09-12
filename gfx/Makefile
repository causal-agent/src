GFX ?= cocoa
PREFIX = ~/.local

BINS = brot gfxx
MAN1 = $(BINS:%=%.1)

CFLAGS += -Wall -Wextra -Wpedantic
LDLIBS = -lm -lz
LDLIBS_cocoa = -framework Cocoa
LDLIBS_x11 = -lX11

all: .gitignore tags $(BINS)

.gitignore: Makefile
	echo '*.o' tags $(BINS) | tr ' ' '\n' > .gitignore

tags: *.h *.c
	ctags -w *.h *.c

brot.o gfxx.o: gfx.h

cocoa.o fb.o x11.o: gfx.h

brot: brot.o $(GFX).o
	$(CC) $(LDFLAGS) brot.o $(GFX).o $(LDLIBS) $(LDLIBS_$(GFX)) -o $@

gfxx: gfxx.o $(GFX).o
	$(CC) $(LDFLAGS) gfxx.o $(GFX).o $(LDLIBS) $(LDLIBS_$(GFX)) -o $@

README: gfx.7
	mandoc gfx.7 | sed $$'s/.\b//g' > README

clean:
	rm -f tags *.o $(BINS)

link:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	ln -s -f $(BINS:%=$(PWD)/%) $(PREFIX)/bin
	ln -s -f $(MAN1:%=$(PWD)/%) $(PREFIX)/share/man/man1

unlink:
	rm -f $(BINS:%=$(PREFIX)/bin/%)
	rm -f $(MAN1:%=$(PREFIX)/share/man/man1/%)
