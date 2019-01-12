PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man

CFLAGS += -std=c99 -Wall -Wextra -DSHELL -Ilibedit
LDLIBS = -lcurses

-include config.mk

SRCS += alias.c
SRCS += arith_yacc.c
SRCS += arith_yylex.c
SRCS += cd.c
SRCS += echo.c
SRCS += error.c
SRCS += eval.c
SRCS += exec.c
SRCS += expand.c
SRCS += histedit.c
SRCS += input.c
SRCS += jobs.c
SRCS += kill.c
SRCS += mail.c
SRCS += main.c
SRCS += memalloc.c
SRCS += miscbltin.c
SRCS += mystring.c
SRCS += options.c
SRCS += output.c
SRCS += parser.c
SRCS += printf.c
SRCS += redir.c
SRCS += show.c
SRCS += test.c
SRCS += trap.c
SRCS += var.c

GENSRCS = builtins.c nodes.c syntax.c
GENHDRS = builtins.h nodes.h syntax.h token.h

SRCS += $(GENSRCS)
OBJS = $(SRCS:.c=.o) libedit/libedit.a

MAN1 = cash.1 cash-kill.1 cash-printf.1 cash-test.1

all: tags cash

cash: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(OBJS): $(GENHDRS)

libedit/libedit.a:
	$(MAKE) -C libedit libedit.a

builtins.c builtins.h: mkbuiltins builtins.def
	sh mkbuiltins .

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes nodetypes nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

token.h: mktokens
	sh mktokens

tags: *.h *.c
	ctags -w *.h *.c

depend: $(SRCS) $(GENHDRS)
	$(CC) $(CFLAGS) -MM $(SRCS) > .depend

-include .depend

clean:
	rm -f cash $(OBJS) mknodes mksyntax $(GENSRCS) $(GENHDRS) tags .depend

install: cash $(MAN1)
	mkdir -p $(PREFIX)/bin $(MANDIR)/man1
	install cash $(PREFIX)/bin
	install -m 644 $(MAN1) $(MANDIR)/man1

uninstall:
	rm -f $(PREFIX)/bin/cash $(MAN1:%=$(MANDIR)/man1/%)
