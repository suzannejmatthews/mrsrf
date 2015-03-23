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

my $count;
my $cols;
my $rows;
my $range1;
my $range2;
my @all_trees;
my $dir;

if ($#ARGV != 2) {
    print "usage: perl create_final_matrix.pl <dir> <num nodes> <file name id>\n";
  print "stiches sub-matrices outputted by MrsRF located in <dir>\n";
  print "into global RF matrix.\n";
  print "WARNING: Only supports up to 8 nodes!\n";
  exit 1;
}


my $dir = $ARGV[0];
my $nodes = $ARGV[1];
my $filename = $ARGV[2];
#print "nodes is: $nodes\n";

if ($nodes > 8) { 
    print "ERROR: Too many nodes! Not yet supported\n";
    exit 1;
}

if ($nodes == 1) { 
    #print "nothing to do. Exiting...\n";
    exit 0;
}

if ($nodes == 2) {
    #print "we get here!\n"; 
   open(OUT, ">$dir/RFMatrix.txt");
    my $file = $filename."0.txt";
    my $file1 = $filename."1.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    my $readline;
    my $readline2;
    #print "we get here too!\n"; 
   while ($readline = <IN>) {
       $readline2 = <IN2>;
       chomp $readline;
       chomp $readline2;
       #print "$readline$readline2\n";
       print OUT "$readline$readline2\n";
   }
   close(IN);
   close(IN2);
   close(OUT);
   exit 0;

}
if ($nodes == 4) { 
    open(OUT, ">RFMatrix.txt");
    my $file = $filename."0.txt";
    my $file1 = $filename."1.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    my $readline;
    my $readline2;
    while ($readline = <IN>) {
	$readline2 = <IN2>;
	chomp $readline;
	chomp $readline2;
	$readline =~ s/[\x0A\x0D]+/\n/g;
	if ($readline eq "") {last;}
	print OUT "$readline$readline2\n";

    }
    close(IN);
    close(IN2);
    my $file = $filename."2.txt";
    my $file1 = $filename."3.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    while ( $readline = <IN>){
	$readline2 = <IN2>;
	chomp $readline;
	chomp $readline2;
	#$readline =~ s/\s+$//;
	$readline2 =~ s/\s+$//;
	print OUT "$readline$readline2\n";
    }
    close(IN);
    close(IN2);
    close(OUT);
    exit 0;
}
if ($nodes == 6){ 
    open(OUT, ">RFMatrix.txt");
    my $file = $filename."0.txt";
    my $file1 = $filename."1.txt";
    my $file2 = $filename."2.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    open(IN3, "<$dir/$file2") or die "cannot open $file2";
    my $readline;
    my $readline2;
    my $readline3;
    while ($readline = <IN>){
	$readline2 = <IN2>;
	$readline3 = <IN3>;
	chomp $readline;
	chomp $readline2;
	chomp $readline3;
	$readline =~ s/[\x0A\x0D]+/\n/g;
	if ($readline eq "") {last;}
	print OUT "$readline$readline2$readline3\n";
    }
    close(IN);
    close(IN2);
    close(IN3);
    my $file = $filename."3.txt";
    my $file1 = $filename."4.txt";
    my $file2 = $filename."5.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    open(IN3, "<$dir/$file2") or die "cannot open $file2";
    while ($readline = <IN>){
	$readline2 = <IN2>;
	$readline3 = <IN3>;
	chomp $readline;
	chomp $readline2;
	chomp $readline3;
	print OUT "$readline$readline2$readline3\n";
    }
    close(IN);
    close(IN2);
    close(IN3);
    close(OUT);
    exit 0;
}
if ($nodes == 8) { 
    open(OUT, ">RFMatrix.txt");
    my $file = $filename."0.txt";
    my $file1 = $filename."1.txt";
    my $file2 = $filename."2.txt";
    my $file3 = $filename."3.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    open(IN3, "<$dir/$file2") or die "cannot open $file2";
    open(IN4, "<$dir/$file3") or die "cannot open $file3";
   
    my $readline;
    my $readline2;
    my $readline3;
    my $readline4;
    while ($readline = <IN>) {
	$readline2 = <IN2>;
	$readline3 = <IN3>;
	$readline4 = <IN4>;
	chomp $readline;
	chomp $readline2;
	chomp $readline3;
	chomp $readline4;
	$readline =~ s/[\x0A\x0D]+/\n/g;
	if ($readline eq "") {last;}
	print OUT "$readline$readline2$readline3$readline4\n";
    }
    close(IN);
    close(IN2);
    close(IN3);
    close(IN4);
    my $file = $filename."4.txt";
    my $file1 = $filename."5.txt";
    my $file2 = $filename."6.txt";
    my $file3 = $filename."7.txt";
    open(IN, "<$dir/$file") or die "cannot open $file";
    open(IN2, "<$dir/$file1") or die "cannot open $file1";
    open(IN3, "<$dir/$file2") or die "cannot open $file2";
    open(IN4, "<$dir/$file3") or die "cannot open $file3";
    
    while ( $readline = <IN>) {
	$readline2 = <IN2>;
	$readline3 = <IN3>;
	$readline4 = <IN4>;
	chomp $readline;
	chomp $readline2;
	chomp $readline3;
	chomp $readline4;
	print OUT "$readline$readline2$readline3$readline4\n";
    }
    close(IN);
    close(IN2);
    close(IN3);
    close(IN4);
    close(OUT);
    exit 0;
}

print "ERROR! Node number not supported!\n";
exit 2;

