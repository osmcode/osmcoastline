#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid coastline with "inland sea".
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"

    0--------1----\
   /               \--2\
   3     4------5       6
    \     \7--8/       /
     \                /
      -9------------a

NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn100,n103,n109,n110,n106,n102,n101,n100
w201 v1 Tnatural=coastline Nn104,n105,n108,n107,n104
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

if [ "$SRID" = "4326" ]; then
    test $RC -eq 1
    grep '^There were 1 warnings.$' "$LOG"
    check_count error_lines 1;
else
    # "questionables" are not checked in 3857
    test $RC -eq 0
    grep '^There were 0 warnings.$' "$LOG"
    check_count error_lines 0;
fi

grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;

echo "SELECT InsertEpsgSrid(4326);" | $SQL

echo "SELECT AsText(Transform(geometry, 4326)) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.05 1.99, 1.14 1.99, 1.23 1.98, 1.25 1.97, 1.21 1.94, 1.08 1.94, 1.04 1.97, 1.05 1.99), (1.1 1.97, 1.12 1.96, 1.15 1.96, 1.17 1.97, 1.1 1.97))'

if [ "$SRID" = "4326" ]; then
    echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
        | grep -F 'LINESTRING(1.1 1.97, 1.17 1.97, 1.15 1.96, 1.12 1.96, 1.1 1.97)|201|questionable'
fi

#-----------------------------------------------------------------------------
