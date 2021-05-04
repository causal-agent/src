#!/usr/bin/env perl
use strict;
use warnings;

while (<>) {
	print;
	last if /^[.]Sh STANDARDS$/;
}

my ($ref, @refs);
while (<>) {
	next if /^[.](Bl|It|$)/;
	last if /^[.]El$/;
	if (/^[.]Rs$/) {
		$ref = {};
	} elsif (/^[.]%(.) (.*)/) {
		$ref->{$1} = [] unless $ref->{$1};
		push @{$ref->{$1}}, $2;
	} elsif (/^[.]Re$/) {
		push @refs, $ref;
	} else {
		print;
	}
}

sub byLast {
	my ($af, $al) = split /\s(\S+)(,.*)?$/, $a;
	my ($bf, $bl) = split /\s(\S+)(,.*)?$/, $b;
	($al // $af) cmp ($bl // $bf) || $af cmp $bf;
}

foreach $ref (@refs) {
	@{$ref->{A}} = sort byLast @{$ref->{A}};
	@{$ref->{Q}} = sort @{$ref->{Q}} if $ref->{Q};
	if ($ref->{N} && $ref->{N}[0] =~ /RFC/) {
		$ref->{R} = $ref->{N};
		delete $ref->{N};
	}
	if ($ref->{R} && $ref->{R}[0] =~ /RFC (\d+)/ && !$ref->{U}) {
		$ref->{U} = ["https://tools.ietf.org/html/rfc${1}"];
	}
}

sub byAuthor {
	local ($a, $b) = ($a->{A}[0], $b->{A}[0]);
	byLast();
}

{
	local ($,, $\) = (' ', "\n");
	print '.Bl', '-item';
	foreach $ref (sort byAuthor @refs) {
		print '.It';
		print '.Rs';
		foreach my $key (qw(A T B I J R N V U P Q C D O)) {
			next unless $ref->{$key};
			foreach (@{$ref->{$key}}) {
				print ".%${key}", $_;
			}
		}
		print '.Re';
	}
	print '.El';
}

while (<>) {
	print;
}
