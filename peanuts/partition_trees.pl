#!/usr/bin/perl

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



###############################
## File: partition_trees.pl
###############################

use strict;
use File::Copy;

if ($#ARGV != 5) {
  print "usage: perl partition_trees.pl <tree file> <num trees> <num nodes>";
  print " <node id> <num cores> <directory>\n";
  print "splits tree file into <num cores>, and generates input-<node id>.txt file\n\n";
  print "How to read input-<node id>.txt:\n";
  print "F1 <size of F1> <offset 1> <marker 1>\n";
  print "F2 <size of F2> <offset 1> <marker 1>\n";
  print "FC <size of FC> <offset c> <marker c>\n";
  print "where c is the number of cores, <offset> denotes the tree index and";
  print "<marker> differentiates file origins (+/- 1)\n";
  print "all files and input-<node id>.txt will be written to directory <directory>\n";
  exit 1;
}

my $infile = $ARGV[0];
my $size = $ARGV[1];
my $nodes = $ARGV[2];
my $n_id = $ARGV[3];
my $cores = $ARGV[4];
my $dir = $ARGV[5];

##################################
###Vacuous Checks
##################################
if ($nodes < 1) { 
    print "ERROR! Number of nodes must be greater than 1!\n";
    exit 1;
}
if ($cores != 1) { 
    if ($cores % 2 != 0) {
	print "ERROR! Number of cores must be a multiple of 2!\n";
	exit 1;
    }
}
if ($nodes < $n_id) { 
    my $temp = $nodes-1;
    print "ERROR! Invalid node id $n_id!. Must be in range of 0...$temp\n";
    exit 1;
}
if ($nodes == $n_id) { 
    my $temp = $nodes-1;
    print "ERROR! Invalid node id $n_id!. Must be in range of 0...$temp\n";
    exit 1;
}
##################################
### Determine nodal configuration
##################################


my $num_elems = 1;
if ($cores != 1) {
    $num_elems = $cores/2;
} 

my @pstart;
my @pend;
my @qstart;
my @qend;

my $gpstart;
my $gpend;
my $gqstart;
my $gqend;
my $y;
my $x; 
my $x_loc;
my $y_loc;

if ($nodes == 1) { 
    $gpstart = 0;
    $gpend = $size;
    $gqstart = 0;
    $gqend = $size;
    $x_loc = 0;
    $y_loc = 0;
}
else { 
    my $max = sqrt($nodes);
    print "square root is: $max\n";
    my $natural = int($max);
    if ($natural == $max) { 
	print "perfect square root!\n";
	print "file configuration: $natural x $natural\n";
	$y = $natural;
	$x = $natural;
    }
    else { 
	print "not perfect square root... where to start: $natural\n";
	while ($natural > 0) { 
	    if ($nodes%$natural == 0) { 
		$y = $nodes/$natural;
		$x = $natural;
		last;
	    }
	    else{
		$natural-=1;
	    }
	}	
	if ($natural == 0){ 
	    print "ERROR: fatal flaw in partitioning!\n";
	    exit 1;
	}
	print "file configuration: $x x $y\n";
    }
    $x_loc = int($n_id / $y);
    $y_loc = $n_id % $y;

   #determine start and end locations for "subfiles" p and q
   #determine start and end location for file p
    my $remainder = $size % $x;
    my $num_trees_pfile;
    if ($remainder == 0) { #then all the files are of equal size
	$num_trees_pfile = $size / $x;
	$gpstart = $x_loc*$num_trees_pfile;
	$gpend = $gpstart + $num_trees_pfile; 
    }
    else {
	$num_trees_pfile = int($size/$x);
	if ($x_loc < $remainder) {
	    $gpstart = $x_loc*($num_trees_pfile + 1);
	    $gpend = $gpstart + $num_trees_pfile + 1;
	
	}
	else { 
	    my $diff = $x_loc - $remainder;
	    $gpstart = $remainder*($num_trees_pfile + 1);
	    $gpstart += $diff*($num_trees_pfile);
	    $gpend = $gpstart + $num_trees_pfile;
	}
    }
    #do again for determining q
    $remainder = $size % $y;
    #print "remainder is: $remainder\n";
    if ($remainder == 0) { #then all the files are of equal size
	$num_trees_pfile = $size / $y;
	$gqstart = $y_loc*$num_trees_pfile;
	$gqend = $gqstart + $num_trees_pfile; 
    }
    else {
	$num_trees_pfile = int($size/$y);
	if ($y_loc < $remainder) {
	    $gqstart = $y_loc*($num_trees_pfile + 1);
	    $gqend = $gqstart + $num_trees_pfile + 1;
	}
	else{ 
	    my $diff = $y_loc - $remainder;
	    $gqstart = $remainder*($num_trees_pfile + 1);
	    $gqstart += $diff*($num_trees_pfile);
	    $gqend = $gqstart + $num_trees_pfile;
	}
    }

}

##################################
### Fill core splitting indices
##################################

#Next step: use four global p and q variables to determine how to split trees specified
#by respective variables into c/2 files each.

my $ptrees = $gpend - $gpstart;
my $qtrees = $gqend - $gqstart;
my $true_cores = 0;

if ($cores == 1) {
    $cores = 2;
} 
my $num_files_each = $cores/2;
#we first determine the indices for the p file
my $remainder = $ptrees % $num_files_each;
my $trees_pfile = int($ptrees / $num_files_each);
if ($remainder == 0) { #if we can split it evenly
    #fill indices evenly
    for (my $i = 0; $i < $ptrees; $i+=$trees_pfile) { 
	my $true_i = $gpstart + $i;
	push(@pstart, $true_i);
	my $end = $true_i + $trees_pfile;
	push(@pend, $end);
    }
}
else{
    my $limit = $trees_pfile + 1;
    my $i=0;
    my $count=0;
    while ($count < $remainder){ 
	my $true_i = $gpstart + $i;
	push(@pstart, $true_i);
	my $end = $true_i + $limit;
	push (@pend, $end);
	$i +=$limit;
	$count++;
    }
    for ($i; $i < $ptrees; $i+=$trees_pfile) {
	my $true_i = $gpstart + $i; 
	push(@pstart, $true_i);
	my $end = $true_i + $trees_pfile;
	push(@pend, $end);
    }
}

#we then determine the indices for the q file
my $remainder = $qtrees % $num_files_each;
my $trees_pfile = int($qtrees / $num_files_each);
if ($remainder == 0) { #if we can split it evenly
    #fill indices evenly
    for (my $i = 0; $i < $qtrees; $i+=$trees_pfile) { 
	my $true_i = $gqstart + $i;
	push(@qstart, $true_i);
	my $end = $true_i + $trees_pfile;
	push(@qend, $end);
    }
}
else{
    my $limit = $trees_pfile + 1;
    my $i=0;
    my $count=0;
    while ($count < $remainder){ 
	my $true_i = $gqstart + $i;
	push(@qstart, $true_i);
	my $end = $true_i + $limit;
	push (@qend, $end);
	$i +=$limit;
	$count++;
    }
    for ($i; $i < $qtrees; $i+=$trees_pfile) {
	my $true_i = $gqstart + $i; 
	push(@qstart, $true_i);
	my $end = $true_i + $trees_pfile;
	push(@qend, $end);
    }
}

print "Stats so far:\n";
print "Total number of trees is: $size\n";
print "Total number of nodes is: $nodes\n";
print "Node id: $n_id\n x-loc: $x_loc y-loc: $y_loc\n";
print "corresponding p file: trees [$gpstart .. $gpend)\n";
print "corresponding q file: trees [$gqstart .. $gqend)\n";
print "number of total trees for p: $ptrees\n";
print "number of total trees for q: $qtrees\n";

my $psize = scalar(@pstart);
my $qsize = scalar(@qstart);
if ($psize != $num_files_each) { 
    print "ERROR! p indices don't match required number of files! (p is: $psize, not $num_files_each)\n";
    exit 1;
}
if ($qsize != $num_files_each) { 
    print "ERROR! q indices don't match required number of files! (q is: $qsize, not $num_files_each)\n";
    exit 1;
}
print "p is going to be split into $psize files\n";
print "Files: ";
for (my $i = 0; $i < $psize; ++$i) { 
    print "[$pstart[$i] .. $pend[$i]) ";
}
print "\n";

print "q is going to be split into $qsize files\n";
print "Files: ";
for (my $i = 0; $i < $qsize; ++$i) { 
    print "[$qstart[$i] .. $qend[$i]) ";
}
print "\n";

my $start = 0;
my $end = 0;

if ($qstart[0] < $pstart[0]) { 
    $start = $qstart[0];
}
else{ 
    $start = $pstart[0];
}

if ($qend[$num_files_each-1] > $pend[$num_files_each-1]) { 
    $end = $qend[$num_files_each-1];
}
else { 
    $end = $pend[$num_files_each-1];
}

print "Thus, we only care about trees in the range: [$start .. $end)\n";

##################################
### Last step: 
### Begin processing file
##################################
#copy input file into specified directory.
my $newfile = `basename $infile`;
unless (-d $dir){
    mkdir($dir, 0777) || die "can't create directory!";
}
`cat $infile > $dir/$newfile`;
open(NODE, ">$dir/node-$n_id.size")  || die;
print NODE "$ptrees x $qtrees\n";
close(NODE);;
#chdir($dir);
my $readline; 
open(IN, "$dir/$newfile") || die "cannot open file $newfile!\n";
my $line = 0;
my $p_id = 0;
my $q_id = 0;
open(OUTp,">$dir/trees-p.$p_id") || die "cannot open trees-p.$p_id!";
open(OUTq,">$dir/trees-q.$q_id") || die "cannot open trees-q.$q_id!";
print "start is: $start\n";
print "end is: $end\n";
while ($readline = <IN>) { 
#    print "do we get here?\n";
    if ($line == $end) { #executed if there is a blank line
	print "generating input-$n_id.txt...\n";
	open(OUT, ">$dir/input-$n_id.txt");
	for (my $i = 0; $i < $num_files_each; ++$i) { 
	    my $pfsize = $pend[$i] - $pstart[$i];
	    my $qfsize = $qend[$i] - $qstart[$i];
	    my $offset_p = $pstart[$i] - $pstart[0];
	    my $offset_q = $qstart[$i] - $qstart[0];
	    print OUT "trees-p.$i $pfsize $offset_p +1\n";
	    print OUT "trees-q.$i $qfsize $offset_q -1\n";
	}
	close(OUT);
	close(IN);
	exit 0;
    }
    if ($line >= $start){ #if we are at our starting point
	
	chomp $readline;
	if ($line == $pend[$p_id]) { #if we are done processing current p
	    #print "close trees-p-$p_id\n"; 
	    close(OUTp); #close the file
	    $p_id++;
	    if ($p_id < $num_files_each) { #if we are not done processing all of p
		#print "open trees-p-$p_id\n";
		open(OUTp,">$dir/trees-p.$p_id") || die "ERROR!";
	    }
	    else { #if we are done processng all of p, say so.
		print "done processing p\n";
	    }
	}
	if ($line == $qend[$q_id]) {
	    #print "close trees-q-$q_id\n";
	    close(OUTq); #close once we're done processing current q
	    $q_id++; 
	    if ($q_id < $num_files_each) {#if we have more to process
		#print "open trees-q-$q_id\n"; #open new q file
		open(OUTq,">$dir/trees-q.$q_id") || die "ERROR!"; 
	    }
	    else {#else, say we are done.
		print "done processing q\n";
	    }
	}
	if ( ($line >= $pstart[$p_id]) && ($line < $pend[$p_id])) {
            #if in p range 
	    #print "line $line will be printed out to output-p-$p_id\n";
	    print OUTp "$readline\n";
	}

	if ( ($line >= $qstart[$q_id]) && ($line < $qend[$q_id])) {
            #if in q range 
	    #print "line $line will be printed out to output-q-$q_id\n";
	    print OUTq "$readline\n";
	}

	
    }
    #print "line: $line\n";
    $line++;
}
close(IN);

if ($line == $end) { #if there is no blank line at end of file
    print "generating input-$n_id.txt...\n";
    open(OUT, ">$dir/input-$n_id.txt");
    for (my $i = 0; $i < $num_files_each; ++$i) { 
	my $pfsize = $pend[$i] - $pstart[$i];
	my $qfsize = $qend[$i] - $qstart[$i];
	my $offset_p = $pstart[$i] - $pstart[0];
	my $offset_q = $qstart[$i] - $qstart[0];
	print OUT "trees-p.$i $pfsize $offset_p +1\n";
	print OUT "trees-q.$i $qfsize $offset_q -1\n";
    }
    close(OUT);
    print "done.\n";
}
else { 
    $line--;
    print "FILE DOESN'T HAVE ENOUGH LINES!!! $line lines found!\n";
    exit 1;
}
