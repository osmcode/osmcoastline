#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Node IDs don't match but same location
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x1.00 y1.00
n101 v1 x1.00 y1.01
n102 v1 x1.01 y1.01
n103 v1 x1.01 y1.00
n104 v1 x1.00 y1.00
w200 v1 Tnatural=coastline Nn100,n101,n102
w201 v1 Tnatural=coastline Nn102,n103,n104
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'There are 2 nodes where the coastline is not closed.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 2 errors.$' "$LOG"

check_count land_polygons 0;
check_count error_points 2;
check_count error_lines 0;

echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL >"$DUMP"

grep -F 'POINT(1 1)|100|fixed_end_point' "$DUMP"
grep -F 'POINT(1 1)|104|fixed_end_point' "$DUMP"

#-----------------------------------------------------------------------------
