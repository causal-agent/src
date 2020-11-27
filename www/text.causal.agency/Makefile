WEBROOT = /usr/local/www/text.causal.agency

TXTS += 001-make.txt
TXTS += 002-writing-mdoc.txt
TXTS += 003-pleasant-c.txt
TXTS += 004-uloc.txt
TXTS += 005-testing-c.txt
TXTS += 006-some-libs.txt
TXTS += 007-cgit-setup.txt
TXTS += 008-how-irc.txt
TXTS += 009-casual-update.txt
TXTS += 010-irc-suite.txt
TXTS += 011-libretls.txt
TXTS += 012-inability.txt

all: ${TXTS}

.SUFFIXES: .7 .txt

.7.txt:
	mandoc -T utf8 $< | col -bx > $@

feed.atom: feed.sh ${TXTS}
	sh feed.sh > feed.atom

clean:
	rm -f ${TXTS} feed.atom

install: ${TXTS} feed.atom
	install -p -m 644 ${TXTS} feed.atom ${WEBROOT}
