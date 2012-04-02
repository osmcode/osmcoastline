#!/bin/sh

rm -f testdata.db
#valgrind --leak-check=full --show-reachable=yes
./osmcoastline --debug --verbose --output-rings -o testdata.db testdata.osm

