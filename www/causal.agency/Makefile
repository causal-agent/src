WEBROOT = /usr/local/www/causal.agency

FILES = index.html scheme.png

all: $(FILES)

install: $(FILES)
	install -C -m 644 $(FILES) $(WEBROOT)

INCLUDES = scheme.css torus.html play.html catgirl.html

index.html: index.html.in index.sed $(INCLUDES)
	sed -f index.sed index.html.in > index.html

FLAGS_catgirl.pty = -c
FLAGS_torus.pty = -h 25

.SUFFIXES: .html .pty

.pty.html:
	shotty $(FLAGS_$<) $< > $@

scheme.css: scheme.sed
	scheme -s | sed -f scheme.sed > scheme.css

scheme.png:
	scheme -g > scheme.png

clean:
	rm -f *.html scheme.css scheme.png
