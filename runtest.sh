#!/bin/sh

#valgrind --leak-check=full --show-reachable=yes
./osmcoastline --debug --verbose --overwrite --output-rings -o testdata.db testdata.osm

