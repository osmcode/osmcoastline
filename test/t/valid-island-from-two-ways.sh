#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid small "island" made up from two ways with coastline all around.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 x1.01 y1.01
n101 v1 x1.02 y1.01
n102 v1 x1.03 y1.02
n103 v1 x1.04 y1.02
n104 v1 x1.05 y1.03
n105 v1 x1.01 y1.03
w200 v1 Tnatural=coastline Nn100,n101,n102
w201 v1 Tnatural=coastline Nn102,n103,n104,n105,n100
OSM

#-----------------------------------------------------------------------------

set -e

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1

test $? -eq 0

grep 'There are 1 coastline rings (0 from a single closed way and 1 others).$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 0;
check_count error_lines 0;

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.01 1.01, 1.01 1.03, 1.05 1.03, 1.04 1.02, 1.03 1.02, 1.02 1.01, 1.01 1.01))'

#-----------------------------------------------------------------------------
