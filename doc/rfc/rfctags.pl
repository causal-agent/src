#!/usr/bin/env perl
use strict;
use warnings;
use open ':encoding(ISO-8859-1)';

($,, $\) = ("\t", "\n");
while (<>) {
	chomp;
	# Section headings
	if (/^([\d.]+|[A-Z][.])\s+([^\t]+)?/) {
		print $1, $ARGV, $.;
		print $2, $ARGV, $. if $2;
		print $1, $ARGV, $. if $1 =~ /^([\d.]+)[.]$/;
	}
	# References
	if (/^\s*(\[[\w-]+\])\s{2,}/) {
		print $1, $ARGV, $.;
		print "\\$1", $ARGV, $.; # vim ^] prepends \ to [
	}
	close ARGV if eof;
}
