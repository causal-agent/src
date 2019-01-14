CFLAGS += -std=c99 -Wall -Wextra

-include config.mk

OBJS += chared.o
OBJS += chartype.o
OBJS += common.o
OBJS += el.o
OBJS += eln.o
OBJS += emacs.o
OBJS += filecomplete.o
OBJS += hist.o
OBJS += history.o
OBJS += historyn.o
OBJS += keymacro.o
OBJS += literal.o
OBJS += map.o
OBJS += parse.o
OBJS += prompt.o
OBJS += read.o
OBJS += readline.o
OBJS += refresh.o
OBJS += search.o
OBJS += sig.o
OBJS += terminal.o
OBJS += tokenizer.o
OBJS += tokenizern.o
OBJS += tty.o
OBJS += vi.o

AHDR = common.h emacs.h vi.h
ASRC = common.c emacs.c vi.c

libedit.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(OBJS): $(AHDR) fcns.h func.h help.h

common.h: makelist common.c
	sh makelist -h common.c > common.h

emacs.h: makelist emacs.c
	sh makelist -h emacs.c > emacs.h

vi.h: makelist vi.c
	sh makelist -h vi.c > vi.h

fcns.h: makelist $(AHDR)
	sh makelist -fh $(AHDR) > fcns.h

func.h: makelist $(AHDR)
	sh makelist -fc $(AHDR) > func.h

help.h: makelist $(ASRC)
	sh makelist -bh $(ASRC) > help.h

clean:
	rm -f libedit.a $(OBJS) $(AHDR) fcns.h func.h help.h
