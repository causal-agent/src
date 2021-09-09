#!/usr/bin/env perl

# FIXME?
use lib (split(/:/, $ENV{GITPERLLIB} || '/opt/local/share/perl5'));

use strict;
use warnings;
use Getopt::Long qw(:config pass_through);
use Git;

my $repo = Git->repository();

my $all = 0;
my $commentStart = $repo->config('comment.start') // "/*";
my $commentLead = $repo->config('comment.lead') // " *";
my $commentEnd = $repo->config('comment.end') // " */";
my $minGroup = $repo->config('comment.minGroup') // 2;
my $minRepeat = $repo->config('comment.minRepeat') // 20;
my $noRepeat = $repo->config_bool('comment.noRepeat');
GetOptions(
	'all' => \$all,
	'comment-start=s' => \$commentStart,
	'comment-lead=s' => \$commentLead,
	'comment-end:s' => \$commentEnd,
	'min-group=i' => \$minGroup,
	'min-repeat=i' => \$minRepeat,
	'no-repeat' => \$noRepeat,
) or die;

sub printComment {
	my ($indent, $abbrev, $summary, @body) = @_;
	print "$indent$commentStart $abbrev $summary";
	if (@body) {
		print "\n$indent$commentLead\n";
		foreach (@body) {
			print "$indent$commentLead";
			print " $_" if $_;
			print "\n";
		}
		print "$indent$commentEnd\n" if $commentEnd;
	} else {
		print "$commentEnd\n";
	}
}

my ($pipe, $ctx) = $repo->command_output_pipe('blame', '--porcelain', @ARGV);

my ($commit, $nr, $group, $printed, %abbrev, %summary, %body, %nrs);
while (<$pipe>) {
	chomp;
	if (/^([[:xdigit:]]+) \d+ (\d+) (\d+)/) {
		($commit, $nr, $group, $printed) = ($1, $2, $3, 0);
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
		unless ($printed) {
			$printed = 1;
			if (
				$group >= $minGroup &&
				!($noRepeat && $nrs{$commit}) &&
				!($nrs{$commit} && $nr < $nrs{$commit} + $minRepeat) &&
				($all || @{$body{$commit}})
			) {
				$nrs{$commit} = $nr;
				printComment(
					$indent, $abbrev{$commit}, $summary{$commit},
					@{$body{$commit}}
				);
			}
		}
		print "$indent$line\n";
	}
}

$repo->command_close_pipe($pipe, $ctx);
