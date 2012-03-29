#!/bin/sh

rm -f testdata.db
#valgrind --leak-check=full --show-reachable=yes
./osmcoastline -d -R -o testdata.db testdata.osm

