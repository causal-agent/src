MODULE = ftp.rfc-editor.org::rfcs-text-only

tags: rfctags.pl
	perl rfctags.pl > $@

sync:
	rsync -z ${MODULE}/rfc-index.txt ${MODULE}/'rfc[0-9]*.txt' .

compress:
	gzip -9f *.txt

clean:
	rm -f *.txt *.txt.gz tags
