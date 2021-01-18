#!/usr/bin/env perl
use strict;
use warnings;
use open ':encoding(ISO-8859-1)';

($,, $\) = ("\t", "\n");
for my $rfc (@ARGV) {
	open my $handle, '<', $rfc or die "${rfc}: $!";
	while (<$handle>) {
		chomp;
		# Section headings
		if (/^([\d.]+|[A-Z][.])\s+([^\t]+)?/) {
			print $1, $rfc, $.;
			print $2, $rfc, $. if $2;
			print $1, $rfc, $. if $1 =~ /^([\d.]+)[.]$/;
		}
		# References
		if (/^\s*(\[[\w-]+\])\s{2,}/) {
			print $1, $rfc, $.;
			print "\\$1", $rfc, $.; # vim ^] prepends \ to [
		}
	}
	die "${rfc}: $!" if $!;
	close $handle;
}
