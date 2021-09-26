CGI_BIN = /var/www/cgi-bin

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic $$(pkg-config --cflags kcgi)
LDLIBS = -static $$(pkg-config --static --libs kcgi-html)

up:

clean:
	rm -f up

install: up
	install up ${CGI_BIN}/up

uninstall:
	rm -f ${CGI_BIN}/up
