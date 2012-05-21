#!/bin/sh

#valgrind --leak-check=full --show-reachable=yes
./osmcoastline --debug --verbose --overwrite --output-lines --output-polygons=both --output-rings -o testdata.db testdata.osm

