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

if ($#ARGV != 5) {
  print "usage: perl split_trees2.pl <tree file1> <tree file2> <num trees 1> <num trees2> <num proc> <dir> \n";
  print "Splits <tree file1> and <tree file2> into <num proc> files each.\n";
  print "HOWTO read input-0.txt file:\n";
  print "file1 file2 size1 size2 offset1 offset2\n";
  exit 1;
}


my $infile1 = $ARGV[0];
my $infile2 = $ARGV[1];
my $size1 = $ARGV[2];
my $size2 = $ARGV[3];
my $num_proc = $ARGV[4];
my $dir = $ARGV[5];
my $file1 = `basename $infile1`;
my $file2 = `basename $infile2`;
print "files to be split: $infile1 and $infile2\n";
chomp $file1;
chomp $file2;
my $readline;

#my $outname1 = $dir."/".$file1;
#my $outname2 = $dir."/".$file2;

my $outname1 = $dir."/"."t1";
my $outname2 = $dir."/"."t2";

my $remainder1 = $size1 % $num_proc;
my $remainder2 = $size2 % $num_proc;

unless (-d "$dir") { 
    #print "creating directory $dir..\n";
    mkdir $dir or die;
}

#print "splitting: $infile1\n";
#print "found: $size1 trees\n";

my @times1;
my @times2;

#if ($infile1 eq $infile2) { 
#    print "Error! Input files must have different names!\n";
#    exit 1;
#}
#print "generating $num_proc files necessary for map reduce process\n";
#if the number of trees can be evenly split by the number of processors
if ($remainder1 == 0 ) {
    my $i;
    my $j = 0;
    my $num_trees_pfile = $size1 / $num_proc;
    my $limit = $num_trees_pfile;
    open(IN, $infile1);
    for ($i = 0; $i < $num_proc; ++$i) { 
        my $temp = $outname1.".$i";
        open(TEMP, ">$temp");
	my $count = 0;
        for ($j; $j < $limit; ++$j) { 
	    $readline = <IN>;
	    chomp $readline;
        print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $limit - $num_trees_pfile;
	push (@times1, $count);
	push(@times1, $printlimit);
        $limit = $limit + $num_trees_pfile;

    }
    close(IN);
}  
else { 
    #print "warning: remainder is non-zero: $remainder1.\n";
    my $i;
    my $j;
    my $num_trees_pfile = int($size1/$num_proc);
    my $newlimit = $num_trees_pfile + 1;

    #print "limit is: $newlimit\n";
    open(IN, $infile1);
    for ($i = 0; $i < $remainder1; ++$i) { 
        my $temp = $outname1.".$i";
        open(TEMP, ">$temp");
	my $count = 0;
        for ($j; $j < $newlimit; ++$j) { 
	    $readline = <IN>;
	    chomp($readline);
            print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $newlimit - $count;
	push(@times1, $count);
	push(@times1, $printlimit);
        $newlimit = $newlimit + $num_trees_pfile + 1;
	
    }
    my $limit = $newlimit - 1;
    for ($i = $remainder1; $i < $num_proc; ++$i) { 
        my $temp = $outname1.".$i";
	my $count;
        open(TEMP, ">$temp");
        for ($j; $j < $limit; ++$j) {
	    $readline = <IN>;
	    chomp($readline); 
            print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $limit - $num_trees_pfile;
	push(@times1, $count);
	push(@times1, $printlimit);
        $limit = $limit + $num_trees_pfile;
    }
    close(IN);
}

#if the number of trees can be evenly split by the number of processors (for file2)
#print "splitting: $infile2\n";
#print "found: $size2 trees\n";
if ($remainder2 == 0 ) {
    my $i;
    my $j = 0;
    my $num_trees_pfile = $size2 / $num_proc;
    my $limit = $num_trees_pfile;
    open(IN, $infile2);
    for ($i = 0; $i < $num_proc; ++$i) { 
        my $temp = $outname2.".$i";
        open(TEMP, ">$temp");
	my $count = 0;
        for ($j; $j < $limit; ++$j) { 
	    $readline = <IN>;
	    chomp $readline;
        print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $limit - $num_trees_pfile;
	push (@times2, $count);
	push(@times2, $printlimit);
        $limit = $limit + $num_trees_pfile;

    }
    close(IN);
}  
else { 
    #print "warning: remainder is non-zero: $remainder2.\n";
    my $i;
    my $j;
    my $num_trees_pfile = int($size2/$num_proc);
    my $newlimit = $num_trees_pfile + 1;

    #print "limit is: $newlimit\n";
    open(IN, $infile2);
    for ($i = 0; $i < $remainder2; ++$i) { 
        my $temp = $outname2.".$i";
        open(TEMP, ">$temp");
	my $count = 0;
        for ($j; $j < $newlimit; ++$j) { 
	    $readline = <IN>;
	    chomp($readline);
            print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $newlimit - $count;
		push(@times2, $count);
		push(@times2, $printlimit);

        $newlimit = $newlimit + $num_trees_pfile + 1;
	
    }
    my $limit = $newlimit - 1;
    for ($i = $remainder2; $i < $num_proc; ++$i) { 
        my $temp = $outname2.".$i";
	my $count;
        open(TEMP, ">$temp");
        for ($j; $j < $limit; ++$j) {
	    $readline = <IN>;
	    chomp($readline); 
            print TEMP "$readline\n";
	    $count++;
        }
        close(TEMP);
	my $printlimit = $limit - $num_trees_pfile;
	push(@times2, $count);
	push(@times2, $printlimit);
        $limit = $limit + $num_trees_pfile;
    }
    close(IN);
}

my $length1 = scalar(@times1);
my $length2 = scalar(@times2);
if ($length1 != $length2) { 
    print "ERROR! times files do not match\n";
    exit 1;
}

#print "length1 is: $length1\n";

open(WRITE2, ">$dir/input-0.txt");
my $count = 0;
my $id = 0;
for (my $x = 0; $x < $length1; ++$x) {
    if ($count % 2 == 0) { 
	#print WRITE2 "$file1.$id $file2.$id";
	print WRITE2 "t1.$id t2.$id";
	$id++; 
    }
    print WRITE2 " $times1[$x] $times2[$x]";
    if ($count % 2 == 1) { 
	print WRITE2 "\n";
    }
    $count++;
}
close(WRITE2);
print "input file located in $dir/input-0.txt\n";


#print "...done.\n";

