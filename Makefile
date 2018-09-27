WEBROOT = /usr/local/www/text.causal.agency

TXTS += 001-make.txt
TXTS += 002-writing-mdoc.txt

all: $(TXTS)

.SUFFIXES: .7 .txt

.7.txt:
	mandoc $< | sed $$'s/.\b//g' > $@

feed.atom: $(TXTS)
	./feed.sh > feed.atom

clean:
	rm -f $(TXTS) feed.atom

install: $(TXTS) feed.atom
	install -p -m 644 $(TXTS) feed.atom $(WEBROOT)
