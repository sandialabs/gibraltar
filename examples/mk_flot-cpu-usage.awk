#!/bin/awk -f
# Using Flot to create performance graphs using Javascript
# Input: output from nmon as paramenter
# Output: Javascript list of metrics for input into the Flot plot.js script.

BEGIN{
    FS = ",";
    time = 0;
    cpu = "";
}

/^CPU/ {
    if ( cpu != $1 ) {
	if ( cpu != "" ) {
	    print "];"
	}
	cpu = $1;
	time = 0;
	print "var "cpu"  = ["
    }
    if ( !( $4 ~ /Sys/ ) )
    print "[\x27"time"\x27 , "$4"],";
    time = time + 1;
}

END { print "];" }
