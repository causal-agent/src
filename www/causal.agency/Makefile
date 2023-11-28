WEBROOT = /var/www/causal.agency

GEN = index.html scheme.css scheme.png
FILES = ${GEN} style.css alpha.html lands.html

all: ${FILES}

.SUFFIXES: .7 .html

.7.html:
	mandoc -T html -O style=style.css $< > $@

scheme.css:
	scheme -st > scheme.css

scheme.png:
	scheme -g > scheme.png

install: ${FILES}
	install -C -m 644 ${FILES} ${WEBROOT}

clean:
	rm -f ${GEN}
