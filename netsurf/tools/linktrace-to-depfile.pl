#!/usr/bin/perl -w

use strict;
use Cwd qw(abs_path);

my %deps;

while (my $line = <>) {
   chomp $line;
   $line =~ s/[()]/ /g;
   for my $word (split(/\s+/, $line)) {
      $deps{abs_path($word)} = 1 if ($word =~ /\.a$/);
   }
}

my @deps = keys %deps;

print join("\t\\\n\t", @deps), "\n";

