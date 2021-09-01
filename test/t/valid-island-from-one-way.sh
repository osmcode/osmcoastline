#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid small "island" with coastline all around.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x80.01 y10.01
n101 v1 x80.04 y10.01
n102 v1 x80.04 y10.04
n103 v1 x80.01 y10.04
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n100
OSM

#-----------------------------------------------------------------------------

set -e

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;
check_count error_lines 0;

echo "SELECT InsertEpsgSrid(4326);" | $SQL

echo "SELECT AsText(Transform(geometry, 4326)) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((80.01 10.01, 80.01 10.04, 80.04 10.04, 80.04 10.01, 80.01 10.01))'

#-----------------------------------------------------------------------------
