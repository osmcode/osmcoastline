#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Self-intersection (closed ring)
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
w200 v1 Tnatural=coastline Nn100,n101,n102,n103,n104,n105,n106,n107,n108,n100
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Self-intersection at or near point' "$LOG"

grep '^There were 1 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 1;
check_count error_points 1;
check_count error_lines 0;

if [ "$SRID" = "4326" ]; then
    echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL \
        | grep -F 'POINT(1.09 1.975)|0|intersection'
fi

#-----------------------------------------------------------------------------
