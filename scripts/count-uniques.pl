#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long qw(GetOptions);

binmode STDIN,  ':encoding(UTF-8)';
binmode STDOUT, ':encoding(UTF-8)';
binmode STDERR, ':encoding(UTF-8)';

my $no_header = 0;
my $reorder = 0;

GetOptions(
    'no-header' => \$no_header,
    'reorder'   => \$reorder,
) or die "Usage: perl count-uniques.pl [--no-header] [--reorder] [input_file]\n";

my $input_file = shift @ARGV;
$input_file = '-' unless defined $input_file;

die "Usage: perl count-uniques.pl [--no-header] [--reorder] [input_file]\n" if @ARGV;

sub split_row {
    my ($line) = @_;

    if (index($line, "\t") >= 0) {
        my @parts = split /\t/, $line, -1;
        s/^\s+|\s+$//g for @parts;
        return @parts;
    }

    return split /\s+/, $line;
}

my %seen_pairs;
my $first_data_row_seen = 0;
my $header_line = q{};
my @scored_rows;
my $row_index = 0;

my $handle;
if ($input_file eq '-') {
    $handle = *STDIN;
}
else {
    open my $file_handle, '<:encoding(UTF-8)', $input_file
      or die "Cannot open $input_file: $!\n";
    $handle = $file_handle;
}

while (my $raw_line = <$handle>) {
    chomp $raw_line;
    my $line = $raw_line;
    $line =~ s/^\s+|\s+$//g;

    next if $line eq q{};

    if (!$no_header && !$first_data_row_seen) {
        $header_line = $raw_line;
        $first_data_row_seen = 1;
        next;
    }

    $first_data_row_seen = 1;

    my @values = split_row($line);
    my $new_pairs = 0;

    for (my $left = 0; $left < @values; ++$left) {
        for (my $right = $left + 1; $right < @values; ++$right) {
            my $pair_key = join "\x1F",
              $left, $values[$left], $right, $values[$right];
            if (!exists $seen_pairs{$pair_key}) {
                $seen_pairs{$pair_key} = 1;
                ++$new_pairs;
            }
        }
    }

    if ($reorder) {
        push @scored_rows, [$new_pairs, $row_index++, $raw_line];
    }
    else {
        print "$raw_line\t# [$new_pairs]\n";
    }
}

if ($reorder) {
    print "$header_line\n" if $header_line ne q{};
    for my $entry (sort { $b->[0] <=> $a->[0] || $a->[1] <=> $b->[1] } @scored_rows) {
        print "$entry->[2]\n";
    }
}
