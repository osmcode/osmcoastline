#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Duplicate coastline
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

"$BIN_DIR/src/nodegrid2opl" << 'NODES' >"$INPUT"
  0--1--2
NODES

cat <<'OSM' >>"$INPUT"
w200 v1 Tnatural=coastline Nn100,n102
w201 v1 Tnatural=coastline Nn100,n101
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'There are 3 nodes where the coastline is not closed.$' "$LOG"

grep '^There were 0 warnings.$' "$LOG"
grep '^There were 3 errors.$' "$LOG"

check_count land_polygons 0;
check_count error_points 2;
check_count error_lines 1;

#-----------------------------------------------------------------------------
