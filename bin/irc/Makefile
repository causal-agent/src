LIBRESSL_PREFIX = /usr/local

CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -I$(LIBRESSL_PREFIX)/include
LDFLAGS += -L$(LIBRESSL_PREFIX)/lib
LDLIBS = -ltls

-include config.mk

relay:

clean:
	rm -f relay
