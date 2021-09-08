#!/usr/bin/env perl

# FIXME?
use lib (split(/:/, $ENV{GITPERLLIB} || '/opt/local/share/perl5'));

use strict;
use warnings;
use Getopt::Long qw(:config pass_through);
use Git;

my $repo = Git->repository();

my $comment_start = $repo->config('comment.start') // "/*";
my $comment_lead = $repo->config('comment.lead') // " *";
my $comment_end = $repo->config('comment.end') // " */";
GetOptions(
	'comment-start=s' => \$comment_start,
	'comment-lead=s' => \$comment_lead,
	'comment-end:s' => \$comment_end,
) or die;

my ($pipe, $ctx) = $repo->command_output_pipe('blame', '--porcelain', @ARGV);

my ($commit, %abbrev, %summary, %body, %printed);
while (<$pipe>) {
	chomp;
	if (/^([[:xdigit:]]+) \d+ \d+ \d+/) {
		$commit = $1;
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
		if (@{$body{$commit}} && !$printed{$commit}) {
			print "$indent$comment_start $abbrev{$commit} $summary{$commit}\n";
			print "$indent$comment_lead\n";
			foreach (@{$body{$commit}}) {
				print "$indent$comment_lead";
				print " $_" if $_;
				print "\n";
			}
			print "$indent$comment_end\n" if $comment_end;
			$printed{$commit} = 1;
		}
		print "$indent$line\n";
	}
}

$repo->command_close_pipe($pipe, $ctx);
