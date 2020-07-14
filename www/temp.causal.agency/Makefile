WEBROOT = /usr/local/www/temp.causal.agency

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -I/usr/local/include
LDFLAGS += -static -L/usr/local/lib
LDLIBS = -lkcgihtml -lkcgi -lz -lmd

up:

clean:
	rm -f up

install: up
	install up ${WEBROOT}/up

uninstall:
	rm -f ${WEBROOT}/up
