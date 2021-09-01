#!/bin/sh
#-----------------------------------------------------------------------------
#
#  This is an especially complex case of the coastline looping back on
#  itself.
#
#-----------------------------------------------------------------------------

# shellcheck source=test/init.sh
. "$1/test/init.sh"

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >"$INPUT"
n100 v1 dV c0 t i0 u T x1.00 y1.07
n101 v1 dV c0 t i0 u T x1.00 y1.06
n102 v1 dV c0 t i0 u T x1.00 y1.03
n103 v1 dV c0 t i0 u T x1.00 y1.02
n104 v1 dV c0 t i0 u T x1.00 y1.05
n105 v1 dV c0 t i0 u T x1.00 y1.04
w200 v1 dV c0 t i0 u Tnatural=coastline Nn101,n100
w201 v1 dV c0 t i0 u Tnatural=coastline Nn100,n101,n104
w202 v1 dV c0 t i0 u Tnatural=coastline Nn103,n102,n105
w203 v1 dV c0 t i0 u Tnatural=coastline Nn105,n104
w204 v1 dV c0 t i0 u Tnatural=coastline Nn104,n105
w205 v1 dV c0 t i0 u Tnatural=coastline Nn104,n101
OSM

#-----------------------------------------------------------------------------

"$OSMC" --verbose --overwrite --srs="$SRID" --output-database="$DB" "$INPUT" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 2

grep '^There were 3 warnings.$' "$LOG"
grep '^There were 2 errors.$' "$LOG"

check_count land_polygons 0;
check_count error_points 2;
check_count error_lines 4;

#-----------------------------------------------------------------------------
