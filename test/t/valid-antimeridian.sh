#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid island crossing the antimeridian.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x-180.0 y1.1
n101 v1 x-179.0 y1.1
n102 v1 x-179.0 y1.4
n103 v1 x-180.0 y1.4
n110 v1 x180.0 y1.4
n111 v1 x179.0 y1.4
n112 v1 x179.0 y1.1
n113 v1 x180.0 y1.1
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n100
w201 v1 Tnatural=coastline Nn110,n111,n112,n113,n110
OSM

#-----------------------------------------------------------------------------

set -e

"$OSMC" --verbose --overwrite --output-lines --output-database="$DB" "$INPUT" >"$LOG" 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 2;
check_count error_points 0;
check_count error_lines 0;

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL >"$DUMP"

grep -F 'POLYGON((-180 1.1, -180 1.4, -179 1.4, -179 1.1, -180 1.1))' "$DUMP"
grep -F 'POLYGON((180 1.4, 180 1.1, 179 1.1, 179 1.4, 180 1.4))' "$DUMP"

echo "SELECT AsText(geometry) FROM lines;" | $SQL >"$DUMP"

grep -F 'LINESTRING(-180 1.4, -179 1.4, -179 1.1, -180 1.1)' "$DUMP"
grep -F 'LINESTRING(180 1.1, 179 1.1, 179 1.4, 180 1.4)' "$DUMP"

#-----------------------------------------------------------------------------
