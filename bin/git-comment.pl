#!/usr/bin/env perl
# Copyright (C) 2021  June McEnroe <june@causal.agency>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

use lib (split(/:/, $ENV{GITPERLLIB} || '/usr/local/share/perl5'));

use strict;
use warnings;
use Getopt::Long qw(:config pass_through);
use Git;

my $repo = Git->repository();

my ($all, $minGroup, $minRepeat, $noRepeat) = (0, 2, 30, 0);
my $commentStart = $repo->config('comment.start') // "/*";
my $commentLead = $repo->config('comment.lead') // " *";
my $commentEnd = $repo->config('comment.end') // " */";
my $pretty = $repo->config('comment.pretty') // 'format:%h %s%n%n%-b';
GetOptions(
	'all' => \$all,
	'comment-start=s' => \$commentStart,
	'comment-lead=s' => \$commentLead,
	'comment-end:s' => \$commentEnd,
	'min-group=i' => \$minGroup,
	'min-repeat=i' => \$minRepeat,
	'no-repeat' => \$noRepeat,
	'pretty=s' => \$pretty,
) or die;

sub printComment {
	my ($indent, $summary, @body) = @_;
	print "$indent$commentStart $summary";
	if (@body) {
		print "\n";
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

my ($commit, $nr, $group, $printed, %message, %nrs);
while (<$pipe>) {
	chomp;
	if (/^([[:xdigit:]]+) \d+ (\d+) (\d+)/) {
		($commit, $nr, $group, $printed) = ($1, $2, $3, 0);
		next if $message{$commit};
		if ($commit =~ /^0+$/) {
			$message{$commit} = ['Not committed yet'];
			next;
		}
		my @message = $repo->command(
			'show', '--no-patch', "--pretty=$pretty", $commit
		);
		$message{$commit} = \@message;
	} elsif (/^\t(\s*)(.*)/) {
		my ($indent, $line) = ($1, $2);
		unless ($printed || $line =~ /^[})]?;?$/) {
			$printed = 1;
			if (
				$group >= $minGroup &&
				!($noRepeat && $nrs{$commit}) &&
				!($nrs{$commit} && $nr < $nrs{$commit} + $minRepeat) &&
				($all || @{$message{$commit}} > 1)
			) {
				$nrs{$commit} = $nr;
				printComment($indent, @{$message{$commit}});
			}
		}
		print "$indent$line\n";
	}
}

$repo->command_close_pipe($pipe, $ctx);
