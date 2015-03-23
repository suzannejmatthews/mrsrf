#*****************************************************************
#
# Copyright (C) 2009 Suzanne Matthews, Tiffani Williams
#              Department of Computer Science & Engineering
#              Texas A&M University
#              Contact: [sjm,tlw]@cse.tamu.edu
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details (www.gnu.org).
#
#*****************************************************************

#!/usr/bin/perl
use strict;
use File::Copy;

my $infile;
my $infile2;
my $diff;

my $readline;
my $readline2;
my @elems1;
my @elems2;

if ($#ARGV != 1) {
  print "usage: perl compare_files.pl <file 1> <file 2>\n";
  print "compares two files. Stops at first difference.\n";
  exit 1;
}

$infile = $ARGV[0];
$infile2 = $ARGV[1];

open(IN, $infile);
open(IN2, $infile2);
open(OUT, ">differences.txt");
print OUT "difference file of $infile v. $infile2\n";
my $count;

$count  = 1;
$diff = 0;

my $totalelems = 0;
my $differences = 0;

# read in all the trees in the file into an array
while ( ($readline = <IN>) && ($readline2 = <IN2>) ) { 
    chomp $readline;
    chomp $readline2;
    $readline =~ s/^\s+//; #remove leading spaces
    $readline2 =~ s/^\s+//; 
    $readline2 =~ s/\s+$//; #remove trailing spaces
    $readline =~ s/\s+$//; 

    @elems1 = split(/s+/, $readline); 
    @elems2 = split(/s+/, $readline2);
    if (scalar(@elems1) != scalar(@elems2)) { 
	print OUT "line $count: $readline $readline2\n";
	$diff = 1;
    }
    else { 
	for (my $i = 0; $i < scalar(@elems1); ++$i) { 
	    $totalelems++;
	    if ($elems1[$i] != $elems2[$i]) { 
		print OUT "difference! $elems1[$i] v. $elems2[$i] ";
		print OUT "found on line $count\n";
		$diff = 1;
		$differences++;
	    }
	}
    }
    $count++;
}

if ($diff == 0) {
    print "files are equivalent\n";
    print OUT "files are equivalent\n";
}
else { 
    print "files are different\n";
    print OUT "$differences out of $totalelems elements are different\n";
    my $percent = ($differences/$totalelems)*100;
    print OUT "$infile and $infile2 are $percent% different\n";
}
close(IN);
close(IN2);
close(OUT);
