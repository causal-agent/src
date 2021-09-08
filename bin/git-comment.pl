#!/usr/bin/env perl

# FIXME?
use lib (split(/:/, $ENV{GITPERLLIB} || '/opt/local/share/perl5'));

use strict;
use warnings;
use Getopt::Long qw(:config pass_through);
use Git;

my $repo = Git->repository();

my $commentStart = $repo->config('comment.start') // "/*";
my $commentLead = $repo->config('comment.lead') // " *";
my $commentEnd = $repo->config('comment.end') // " */";
my $threshold = $repo->config('comment.groupThreshold') // 1;
GetOptions(
	'comment-start=s' => \$commentStart,
	'comment-lead=s' => \$commentLead,
	'comment-end:s' => \$commentEnd,
	'group-threshold=i' => \$threshold,
) or die;

sub printComment {
	my ($indent, $abbrev, $summary, @body) = @_;
	print "$indent$commentStart $abbrev $summary\n";
	print "$indent$commentLead\n";
	foreach (@body) {
		print "$indent$commentLead";
		print " $_" if $_;
		print "\n";
	}
	print "$indent$commentEnd\n" if $commentEnd;
}

my ($pipe, $ctx) = $repo->command_output_pipe('blame', '--porcelain', @ARGV);

my ($commit, $group, $printed, %abbrev, %summary, %body);
while (<$pipe>) {
	chomp;
	if (/^([[:xdigit:]]+) \d+ \d+ (\d+)/) {
		($commit, $group, $printed) = ($1, $2, 0);
		next if $abbrev{$commit};
		my @body = $repo->command(
			'show', '--no-patch', '--pretty=format:%h%n%b', $commit
		);
		$abbrev{$commit} = shift @body;
		$body{$commit} = \@body;
	} elsif (/^summary (.*)/) {
		$summary{$commit} = $1;
	} elsif (/^\t(\s*)(.*)/) {
		my ($indent, $line) = ($1, $2);
		if (@{$body{$commit}} && $group > $threshold && !$printed) {
			printComment(
				$indent, $abbrev{$commit}, $summary{$commit},
				@{$body{$commit}}
			);
			$printed = 1;
		}
		print "$indent$line\n";
	}
}

$repo->command_close_pipe($pipe, $ctx);
