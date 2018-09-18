WEBROOT = /usr/local/www/text.causal.agency

TXTS += 001-make.txt

all: $(TXTS)

.SUFFIXES: .7 .txt

.7.txt:
	mandoc $< | sed $$'s/.\b//g' > $@

clean:
	rm -f $(TXTS)

install: $(TXTS)
	install -p -m 644 $(TXTS) $(WEBROOT)
