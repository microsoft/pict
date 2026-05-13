#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long qw(GetOptions);

binmode STDOUT, ':encoding(UTF-8)';
binmode STDERR, ':encoding(UTF-8)';

my $no_header = 0;

GetOptions(
    'no-header' => \$no_header,
) or die "Usage: perl compare-uniques.pl [--no-header] file_a file_b\n";

die "Usage: perl compare-uniques.pl [--no-header] file_a file_b\n" unless @ARGV == 2;

my ($file_a, $file_b) = @ARGV;

sub split_row {
    my ($line) = @_;

    if (index($line, "\t") >= 0) {
        my @parts = split /\t/, $line, -1;
        s/^\s+|\s+$//g for @parts;
        return @parts;
    }

    return split /\s+/, $line;
}

sub row_key {
    my (@row) = @_;
    return join "\x1F", @row;
}

sub read_rows {
    my ($path, $skip_header) = @_;

    open my $handle, '<:encoding(UTF-8)', $path
      or die "Cannot open $path: $!\n";

    my %counts;
    my %rows_by_key;
    my $first_non_empty_seen = 0;

    while (my $raw_line = <$handle>) {
        chomp $raw_line;
        my $line = $raw_line;
        $line =~ s/^\s+|\s+$//g;

        next if $line eq q{};

        if (!$skip_header && !$first_non_empty_seen) {
            $first_non_empty_seen = 1;
            next;
        }

        $first_non_empty_seen = 1;

        my @row = split_row($line);
        my $key = row_key(@row);
        ++$counts{$key};
        $rows_by_key{$key} = \@row;
    }

    close $handle;

    return (\%counts, \%rows_by_key);
}

sub rows_difference {
    my ($left_counts, $right_counts) = @_;

    my %difference;
    for my $key (keys %{$left_counts}) {
        my $remaining = $left_counts->{$key} - ($right_counts->{$key} // 0);
        $difference{$key} = $remaining if $remaining > 0;
    }

    return \%difference;
}

sub print_difference {
    my ($label, $difference, $rows_by_key) = @_;

    return unless %{$difference};

    print "$label\n";
    for my $key (sort keys %{$difference}) {
        my $row_text = join "\t", @{$rows_by_key->{$key}};
        print "$row_text\t# [$difference->{$key}]\n";
    }
}

my ($rows_a, $rows_a_text) = read_rows($file_a, $no_header);
my ($rows_b, $rows_b_text) = read_rows($file_b, $no_header);

my $match = 1;
for my $key (keys %{$rows_a}, keys %{$rows_b}) {
    if (($rows_a->{$key} // 0) != ($rows_b->{$key} // 0)) {
        $match = 0;
        last;
    }
}

if ($match) {
    my $total_rows = 0;
    $total_rows += $_ for values %{$rows_a};
    print "MATCH: all rows accounted for ($total_rows rows).\n";
    exit 0;
}

print "MISMATCH: files differ.\n";

my $only_a = rows_difference($rows_a, $rows_b);
my $only_b = rows_difference($rows_b, $rows_a);

print_difference('Rows only in first file (or extra count):', $only_a, $rows_a_text);
print_difference('Rows only in second file (or extra count):', $only_b, $rows_b_text);

exit 1;
