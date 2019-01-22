TABLE = default.u

FONTS += sans6x8.psfu
FONTS += sans6x10.psfu
FONTS += sans6x12.psfu
FONTS += sans8x16.psfu
FONTS += serif8x16.psfu

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
