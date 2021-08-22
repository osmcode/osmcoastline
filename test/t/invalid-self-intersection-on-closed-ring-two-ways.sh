#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Self-intersection (closed ring, two ways)
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"
    0         8
         4
       5  3
      2  6    7
    1
NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn103,n104,n105,n106,n107,n108
w200 v1 Tnatural=coastline Nn108,n100,n101,n102,n103
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Self-intersection at or near point' "$LOG"

grep '^There were 1 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 1;
check_count error_lines 0;

echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL \
    | grep -F 'POINT(1.09 1.975)|0|intersection'

#-----------------------------------------------------------------------------
