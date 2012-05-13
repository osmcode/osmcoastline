#!/bin/sh

#valgrind --leak-check=full --show-reachable=yes
./osmcoastline --debug --verbose --overwrite --output-rings --output-lines -o testdata.db testdata.osm

