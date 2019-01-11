CFLAGS += -std=c99 -Wall -Wextra -DSHELL
LDLIBS = -ledit

SHSRCS += alias.c
SHSRCS += arith_yacc.c
SHSRCS += arith_yylex.c
SHSRCS += cd.c
SHSRCS += echo.c
SHSRCS += error.c
SHSRCS += eval.c
SHSRCS += exec.c
SHSRCS += expand.c
SHSRCS += histedit.c
SHSRCS += input.c
SHSRCS += jobs.c
SHSRCS += kill.c
SHSRCS += mail.c
SHSRCS += main.c
SHSRCS += memalloc.c
SHSRCS += miscbltin.c
SHSRCS += mystring.c
SHSRCS += options.c
SHSRCS += output.c
SHSRCS += parser.c
SHSRCS += printf.c
SHSRCS += redir.c
SHSRCS += show.c
SHSRCS += test.c
SHSRCS += trap.c
SHSRCS += var.c

GENSRCS = builtins.c nodes.c syntax.c
GENHDRS = builtins.h nodes.h syntax.h token.h

SRCS = $(SHSRCS) $(GENSRCS)
OBJS = $(SRCS:.c=.o)

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
