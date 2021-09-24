WEBROOT = /usr/local/www/temp.causal.agency

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic $$(pkg-config --cflags kcgi)
LDLIBS = -static $$(pkg-config --static --libs kcgi-html)

up:

clean:
	rm -f up

install: up
	install -m 700 up ${WEBROOT}/up

uninstall:
	rm -f ${WEBROOT}/up
