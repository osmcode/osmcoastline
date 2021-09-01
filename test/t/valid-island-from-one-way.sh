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
n100 v1 x1.01 y1.01
n101 v1 x1.04 y1.01
n102 v1 x1.04 y1.04
n103 v1 x1.01 y1.04
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

if [ "$SRID" = "4326" ]; then
    echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
        | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'
fi

#-----------------------------------------------------------------------------
