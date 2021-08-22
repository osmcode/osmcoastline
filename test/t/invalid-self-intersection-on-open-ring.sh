#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Self-intersection (open ring)
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"

         4
       5  3
      2  6    7
    1

NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn101,n102,n103,n104,n105,n106,n107
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'Self-intersection at or near point' "$LOG"

grep '^There were [12] warnings.$' "$LOG"
grep '^There were 1 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 3;
check_count_with_op error_lines -ge 1;
check_count_with_op error_lines -le 2;

echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL >"$DUMP"

grep -F 'POINT(1.09 1.975)|0|intersection' "$DUMP"
grep -F 'POINT(1.05 1.96)|101|fixed_end_point' "$DUMP"
grep -F 'POINT(1.15 1.97)|107|fixed_end_point' "$DUMP"

#-----------------------------------------------------------------------------
