#!/usr/bin/perl
#
##################################################################
#                                                                #
#  Winpooch : Windows Watchdog                                   #
#  Copyright (C) 2004-2005  Benoit Blanchon                      #
#                                                                #
#  This program is free software; you can redistribute it        #
#  and/or modify it under the terms of the GNU General Public    #
#  License as published by the Free Software Foundation; either  #
#  version 2 of the License, or (at your option) any later       #
#  version.                                                      #
#                                                                #
#  This program is distributed in the hope that it will be       #
#  useful, but WITHOUT ANY WARRANTY; without even the implied    #
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       #
#  PURPOSE.  See the GNU General Public License for more         #
#  details.                                                      #
#                                                                #
#  You should have received a copy of the GNU General Public     #
#  License along with this program; if not, write to the Free    #
#  Software Foundation, Inc.,                                    #
#  675 Mass Ave, Cambridge, MA 02139, USA.                       #
#                                                                #
##################################################################


use warnings ;
use strict ;

my $file = $ARGV[0] ;
my $symbol = $ARGV[1] ;
my $buildcount ;

die "No file specified\n" unless defined $file ;
die "No symbol specified\n" unless defined $symbol ;

system ("mv -f $file $file~") and die ("Failed to rename $file into $file~\n") ;

open IN, "$file~" ;
open OUT, ">$file" ;

while (<IN>)
  {
    if( /$symbol\s+(\d+)/ )
      {
	$buildcount = int($1) + 1 ;
	print OUT "#define $symbol\t$buildcount\n" ;
      }
    else
      { 
	print OUT $_
      }
  }

close OUT ;
close IN ;

print "----------------> BUILD COUNT = $buildcount <----------------\n"
