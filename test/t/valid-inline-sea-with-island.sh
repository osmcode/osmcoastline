#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid coastline with "inland sea" and island in that.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"

    0--------1----\
   /               \--2\
   3     4------5       6
   |     |  bc  |       |
   |     |   d  |       |
    \     \7---8/      /
     \                /
      -9------------a

NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn100,n103,n109,n110,n106,n102,n101,n100
w201 v1 Tnatural=coastline Nn104,n105,n108,n107,n104
w202 v1 Tnatural=coastline Nn111,n112,n113,n111
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'Found 3 rings in input data.$' "$LOG"

grep '^There were 3 warnings.$' "$LOG"
grep '^There were 1 errors.$' "$LOG"

check_count land_polygons 0;
check_count error_points 0;
check_count error_lines 3;

#-----------------------------------------------------------------------------
