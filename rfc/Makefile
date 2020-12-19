tags: rfctags.pl
	perl rfctags.pl > $@

sync:
	rsync -z ftp.rfc-editor.org::rfcs-text-only/'rfc[0-9]*.txt' .

compress:
	gzip -9f *.txt

clean:
	rm -f *.txt *.txt.gz tags
