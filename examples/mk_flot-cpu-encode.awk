#!/bin/awk -f
# Using Flot to create performance graphs using Javascript
# Input: output from benchmark.cc as paramenter
# Output: Javascript list of metrics for input into the Flot plot.js script.

BEGIN{print "var cpu_encode = ["}
{if ( FNR > 4 && $1 %2 == 0 && $2 % 2 == 0) {print "[\x27" $1"/"$2"\x27 , " $6/1024 "],"}}
END{print "];"}
