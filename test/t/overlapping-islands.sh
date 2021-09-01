#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Overlapping islands
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"

      0     1

    2          3
          8        9
    4          5
          a        b
      6     7

NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn100,n102,n104,n106,n107,n105,n103,n101,n100
w200 v1 Tnatural=coastline Nn108,n110,n111,n109,n108
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 1

grep '^There were 2 warnings.$' "$LOG"
grep '^There were 0 errors.$' "$LOG"

check_count land_polygons 2;
check_count error_points 2;
check_count error_lines 0;

echo "SELECT InsertEpsgSrid(4326);" | $SQL

echo "SELECT AsText(Transform(geometry, 4326)), osm_id, error FROM error_points;" | $SQL >"$DUMP"

grep -F 'POINT(1.145 1.94)|0|intersection' "$DUMP"
grep -F 'POINT(1.16 1.96)|0|intersection' "$DUMP"

#-----------------------------------------------------------------------------
