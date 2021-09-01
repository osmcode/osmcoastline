#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Invalid small "island" with coastline going the wrong direction.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x1.01 y1.01
n101 v1 x1.01 y1.04
n102 v1 x1.04 y1.04
n103 v1 x1.04 y1.01
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n100
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Turned 1 polygons around.$' "$LOG"

grep '^There were 1 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;
check_count error_lines 1;

if [ "$SRID" = "4326" ]; then
    echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
        | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'

    echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
        | grep -F 'LINESTRING(1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01)|0|direction'
fi

#-----------------------------------------------------------------------------
