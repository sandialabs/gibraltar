#!/bin/bash
#
# A script that tests a range of parameters for the benchmark program.

backends=(
    "cuda"
    "jerasure"
)

actions=(
    "encode"
    "decode"
)

heading="#k m";
for b in "${backends[@]}"; do
    for a in "${actions[@]}"; do
	heading="${heading} ${b}_${a}"
    done
done
echo "$heading"

for k in $(seq 8 16); do
    for m in $(seq 2 8); do
	results=""
	for b in "${backends[@]}"; do
	    for a in "${actions[@]}"; do
		results="${results} $(./benchmark -k ${k} -m ${m} -i 5 -b ${b} -a ${a})"
	    done
	done
	echo $k $m $results
    done
done
