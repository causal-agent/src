WEBROOT = /var/www/text.causal.agency

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
TXTS += 013-hot-tips.txt
TXTS += 014-using-vi.txt
TXTS += 015-reusing-tags.txt
TXTS += 016-using-openbsd.txt
TXTS += 017-unpasswords.txt
TXTS += 018-operating-systems.txt
TXTS += 019-mailing-list.txt
TXTS += 020-c-style.txt
TXTS += 021-time-machine.txt
TXTS += 022-swans-are-dead.txt
TXTS += 023-sparse-checkout.txt
TXTS += 024-seprintf.txt
TXTS += 025-v6-pwd.txt
TXTS += 026-git-comment.txt
TXTS += 027-openbsd-linode.txt
TXTS += 028-names.txt
TXTS += 029-topics.txt
TXTS += 030-discs.txt
TXTS += 031-books-2021.txt
TXTS += 032-albums-2021.txt
TXTS += 033-jorts.txt
TXTS += 034-voices.txt
TXTS += 035-addendum-2021.txt
TXTS += 036-compassion.txt
TXTS += 037-care.txt
TXTS += 038-agency.txt
TXTS += 039-apologies.txt
TXTS += 040-sound-memory.txt
TXTS += 041-albums-2022.txt
TXTS += 042-comfort-music.txt
TXTS += 043-little-blessings.txt
TXTS += 044-film-review.txt

all: colb ${TXTS}

.SUFFIXES: .7 .fmt .txt

.7.txt:
	mandoc -T utf8 $< | ./colb > $@
	touch -m -r $< $@

.fmt.txt:
	fmt $< | sed '1,/^$$/d' > $@
	touch -m -r $< $@

feed.atom: feed.sh colb ${TXTS}
	sh feed.sh > feed.atom

clean:
	rm -f colb ${TXTS} feed.atom

install: colb ${TXTS} feed.atom
	install -p -m 644 ${TXTS} feed.atom ${WEBROOT}
