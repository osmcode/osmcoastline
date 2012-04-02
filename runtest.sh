#!/bin/sh

rm -f testdata.db
#valgrind --leak-check=full --show-reachable=yes
./osmcoastline -d --verbose -R -o testdata.db testdata.osm

