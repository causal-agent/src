TABLE = default.u

FONTS += sans6x8.psfu

all: $(FONTS)

$(FONTS): $(TABLE)

.SUFFIXES: .psf .psfu

.psf.psfu:
	psfaddtable $< $(TABLE) $@

clean:
	rm -f $(FONTS)
