#!/bin/sh

rm -fr testdata
#valgrind --leak-check=full --show-reachable=yes ./osmcoastline testdata.osm testdata
./osmcoastline testdata.osm testdata

