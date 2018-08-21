BINS = brot gfxx
GFX ?= cocoa

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

clean:
	rm -f tags *.o $(BINS)

link:
	mkdir -p ~/.local/bin
	ln -s -f $(BINS:%=$(PWD)/%) ~/.local/bin

unlink:
	rm -f $(BINS:%=~/.local/bin/%)
