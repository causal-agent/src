WEBROOT = /usr/local/www/causal.agency

FILES = index.html scheme.png

all: ${FILES}

install: ${FILES}
	install -C -m 644 ${FILES} ${WEBROOT}

INCLUDES = scheme.css torus.html play.html catgirl.html scheme.html

index.html: index.html.in index.sed ${INCLUDES}
	sed -f index.sed index.html.in > index.html

FLAGS.torus.pty = -n -h 25
FLAGS.scheme.pty = -n -h 10
FLAGS.play.pty = -h 16

.SUFFIXES: .html .pty

.pty.html:
	shotty ${FLAGS.$<} $< > $@

scheme.css:
	scheme -s > scheme.css

scheme.png:
	scheme -g > scheme.png

clean:
	rm -f ${FILES} ${INCLUDES}
