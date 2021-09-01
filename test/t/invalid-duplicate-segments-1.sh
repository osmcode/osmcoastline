#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Correct coastline created from duplicate segments
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x1.10 y1.06
n101 v1 x1.30 y1.06
n102 v1 x1.35 y1.05
n103 v1 x1.30 y1.04
n104 v1 x1.25 y1.04
n105 v1 x1.20 y1.04
n106 v1 x1.15 y1.04
n107 v1 x1.10 y1.04
n108 v1 x1.05 y1.05
w200 v1 Tnatural=coastline Nn106,n105,n104
w201 v1 Tnatural=coastline Nn106,n105,n104
w202 v1 Tnatural=coastline Nn104,n103,n102,n101,n100,n108,n107,n106
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Turned 0 polygons around.$' "$LOG"

if [ "$SRID" = "4326" ]; then
    grep '^There were 3 warnings.$' "$LOG"
    check_count error_lines 3;
else
    # "questionables" are not checked in 3857
    grep '^There were 2 warnings.$' "$LOG"
    check_count error_lines 2;
fi

grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;

if [ "$SRID" = "4326" ]; then
    echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL >"$DUMP"

    grep -F 'LINESTRING(1.15 1.04, 1.2 1.04)|0|overlap' "$DUMP"
    grep -F 'LINESTRING(1.2 1.04, 1.25 1.04)|0|overlap' "$DUMP"
    grep -F 'LINESTRING(1.15 1.04, 1.2 1.04, 1.25 1.04, 1.3 1.04, 1.35 1.05, 1.3 1.06, 1.1 1.06, 1.05 1.05, 1.1 1.04, 1.15 1.04)|201|questionable' "$DUMP"

    echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
        | grep -F 'POLYGON((1.15 1.04, 1.1 1.04, 1.05 1.05, 1.1 1.06, 1.3 1.06, 1.35 1.05, 1.3 1.04, 1.25 1.04, 1.2 1.04, 1.15 1.04))'
fi

#-----------------------------------------------------------------------------
