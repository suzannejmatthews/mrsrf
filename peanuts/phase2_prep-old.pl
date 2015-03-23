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
my $count;
my $num_proc;
my $num_trees;
my $dir;

if ($#ARGV != 1) {
  print "usage: perl phase2_prep.pl <num proc> <hashtable size> \n";
  #print "generates <num proc> replicas of the hashtable to be used\n";
  exit 1;
}

$num_proc = $ARGV[0];
$num_trees = $ARGV[1];

my $remainder = $num_trees % $num_proc;

#if the number of trees can be evenly split by the number of processors:
#format: filename <total biparts> <start tree id> <num trees>
if ($remainder == 0 ) {
    my $num_trees_pfile = $num_trees / $num_proc;
    my $id = 0;
    open(WRITE2, ">input-0.txt");
    for (my $i = 0; $i < $num_proc; $i++) { 
	print WRITE2 "$i $id $num_trees_pfile\n";
	$id += $num_trees_pfile;
    }
    close(WRITE2);

}  
else { 
    print "warning: remainder is non-zero: $remainder.\n";
    my $num_trees_pfile = int($num_trees/$num_proc);
    my $newlimit = $num_trees_pfile + 1; #replicate remainder times
    #print "limit is: $newlimit\n";
    open(WRITE2, ">input-0.txt");
    my $i;
    my $id = 0;
    for ($i = 0; $i < $remainder; ++$i) { 
	print WRITE2 "$i $id $newlimit\n";
	$id += $newlimit;
    }
    for ($i = $remainder; $i < $num_proc; ++$i) { 
  	print WRITE2 "$i $id $num_trees_pfile\n";
	$id += $num_trees_pfile;
    }
    close(WRITE2);
}
print "input file located in input-0.txt\n";
#system("rm tempdir/*");
print "...done.\n";

