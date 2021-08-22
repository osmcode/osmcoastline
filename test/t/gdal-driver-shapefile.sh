#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Select ESRI Shapefile as the GDAL driver, and output data format.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x1.01 y1.01
n101 v1 x1.04 y1.01
n102 v1 x1.04 y1.04
n103 v1 x1.01 y1.04
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n100
OSM

#-----------------------------------------------------------------------------

set -e

rm -rf "$DB"

"$OSMC" --verbose --overwrite --gdal-driver "ESRI Shapefile" --output-database="$DB" "$INPUT" >"$LOG" 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

test -d "$DB"
test -f "$DB/land_polygons.dbf"
test -f "$DB/land_polygons.prj"
test -f "$DB/land_polygons.shp"
test -f "$DB/land_polygons.shx"

#-----------------------------------------------------------------------------
