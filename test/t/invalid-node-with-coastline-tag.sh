#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Single node with coastline tag is flagged as error.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 x1.01 y1.01 Tnatural=coastline
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'No polygons created!$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 1 errors.$' "$LOG"

check_count land_polygons 0;
check_count error_points 1;
check_count error_lines 0;

echo "SELECT InsertEpsgSrid(4326);" | $SQL

echo "SELECT AsText(Transform(geometry, 4326)), osm_id, error FROM error_points;" | $SQL \
    | grep -F 'POINT(1.01 1.01)|100|tagged_node'

#-----------------------------------------------------------------------------
