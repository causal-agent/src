PREFIX ?= ~/.local

MODULE = ftp.rfc-editor.org::rfcs-text-only

tags: rfctags.pl rfc-index.txt.gz
	perl rfctags.pl | sort -f > $@

rfc-index.txt.gz:
	rsync -ptz ${MODULE}/rfc-index.txt ${MODULE}/'rfc[1-9]*.txt' .
	gzip -9f *.txt

clean:
	rm -f tags *.txt *.txt.gz

install: tags rfc.vim
	install -d ${PREFIX}/share/rfc ${PREFIX}/share/nvim/site/plugin
	ln -f tags *.txt.gz ${PREFIX}/share/rfc
	install -m 644 rfc.vim ${PREFIX}/share/nvim/site/plugin

uninstall:
	rm -fr ${PREFIX}/share/rfc ${PREFIX}/share/nvim/site/plugin/rfc.vim
