#!/bin/bash

output=`dirname $1`/coastlineoutput;
rm -rf $output
mkdir $output

db=${output}/coastline.db;
shp=${output}/${2}_polygons.shp

./build/src/osmcoastline -o $db $1 --output-polygons=$2
ogr2ogr -f "ESRI Shapefile" $shp $db ${2}_polygons
