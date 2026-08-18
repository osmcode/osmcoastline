#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Same as invalid-direction-island-in-bbox, but with a second, correctly
#  mapped island. This means organizePolygons() returns a multipolygon which
#  is handled by a different code path.
#  See https://github.com/osmcode/osmcoastline/issues/41
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"

    0---------------1
    |               |
    |               |         a---b
    |      3--------2         |   |
    |      |                  d---c
    |      |  6---7
    |      |  |   |
    |      |  9---8
    |      |
    5------4

NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn100,n105,n104,n103,n102,n101,n100
w201 v1 Tnatural=coastline Nn106,n107,n108,n109,n106
w202 v1 Tnatural=coastline Nn110,n113,n112,n111,n110
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Turned 1 polygons around.$' "$LOG"

grep '^There were 1 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 3;
check_count error_points 0;
check_count error_lines 1;

echo "SELECT InsertEpsgSrid(4326);" | $SQL

# the island that was turned around
echo "SELECT AsText(Transform(geometry, 4326)) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.15 1.94, 1.19 1.94, 1.19 1.92, 1.15 1.92, 1.15 1.94))'

echo "SELECT AsText(Transform(geometry, 4326)), osm_id, error FROM error_lines;" | $SQL \
    | grep -F '|0|direction'

#-----------------------------------------------------------------------------
