WEBROOT = /usr/local/www/causal.agency

FILES = index.html style.css scheme.css scheme.png

all: ${FILES}

index.html: index.7
	mandoc -T html -O style=style.css index.7 > index.html

scheme.css:
	scheme -st > scheme.css

scheme.png:
	scheme -g > scheme.png

install: ${FILES}
	install -C -m 644 ${FILES} ${WEBROOT}

clean:
	rm -f index.html scheme.css scheme.png
