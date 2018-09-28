TABLE = default.u

FONTS += sans6x8.psfu
PNGS = $(FONTS:psfu=png)

all: $(FONTS)

png: $(PNGS)

$(FONTS): $(TABLE)

.SUFFIXES: .psf .psfu .png

.psf.psfu:
	psfaddtable $< $(TABLE) $@

.psf.png:
	psf2png $< > $@

clean:
	rm -f $(FONTS) $(PNGS)
