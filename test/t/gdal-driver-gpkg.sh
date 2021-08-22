#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Select "GPKG" as the GDAL driver, and output data format.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x10.01 y1.01
n101 v1 x10.04 y1.01
n102 v1 x10.04 y1.04
n103 v1 x10.01 y1.04
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n100
OSM

#-----------------------------------------------------------------------------

set -e

rm -rf "$DB"

"$OSMC" --verbose --overwrite --gdal-driver "GPKG" --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;
check_count error_lines 0;

#-----------------------------------------------------------------------------
