use strict;
use warnings;
use open ':encoding(ISO-8859-1)';

use IO::Uncompress::Gunzip qw($GunzipError);

($,, $\) = ("\t", "\n");
print '!_TAG_FILE_SORTED', 2, $0; # Promise to pipe this through sort -f
for my $rfc (<*.txt.gz>) {
	my $handle = new IO::Uncompress::Gunzip $rfc
		or die "${rfc}: ${GunzipError}";
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
