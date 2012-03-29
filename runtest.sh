#!/bin/sh

rm -f testdata.db
#valgrind --leak-check=full --show-reachable=yes
./osmcoastline -o testdata.db testdata.osm

