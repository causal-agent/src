CFLAGS += -std=c99 -Wall -Wextra -DSHELL
LDLIBS = -ledit

-include config.mk

OBJS += alias.o
OBJS += arith_yacc.o
OBJS += arith_yylex.o
OBJS += cd.o
OBJS += echo.o
OBJS += error.o
OBJS += eval.o
OBJS += exec.o
OBJS += expand.o
OBJS += histedit.o
OBJS += input.o
OBJS += jobs.o
OBJS += kill.o
OBJS += mail.o
OBJS += main.o
OBJS += memalloc.o
OBJS += miscbltin.o
OBJS += mystring.o
OBJS += options.o
OBJS += output.o
OBJS += parser.o
OBJS += printf.o
OBJS += redir.o
OBJS += show.o
OBJS += test.o
OBJS += trap.o
OBJS += var.o

GENSRCS = builtins.c nodes.c syntax.c
GENHDRS = builtins.h nodes.h syntax.h token.h

OBJS += $(GENSRCS:.c=.o)

cash: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

builtins.c builtins.h: mkbuiltins builtins.def
	sh mkbuiltins .

nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes nodetypes nodes.c.pat

syntax.c syntax.h: mksyntax
	./mksyntax

token.h: mktokens
	sh mktokens

# FIXME
$(OBJS): *.h $(GENHDRS)

clean:
	rm -f cash $(OBJS) mknodes mksyntax $(GENSRCS) $(GENHDRS)
