#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Invalid small "island" with coastline ring not closed.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

cat <<'OSM' >$INPUT
n100 x1.01 y1.01
n101 x1.04 y1.01
n102 x1.04 y1.04
n103 x1.01 y1.04
w200 Tnatural=coastline Nn100,n101,n102,n103
OSM

#-----------------------------------------------------------------------------

$OSMC --verbose --overwrite --output-database=$DB --output-rings $INPUT >$LOG 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'There are 2 nodes where the coastline is not closed.' $LOG
grep 'Closed 1 rings. This left 0 nodes where the coastline could not be closed.' $LOG
grep '^There were 0 warnings.$' $LOG
grep '^There were 1 errors.$' $LOG

check_count land_polygons 1;
check_count rings 1;
check_count error_points 2;
check_count error_lines 1;

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'

echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL >$DUMP

grep -F 'POINT(1.01 1.01)|100|fixed_end_point' $DUMP
grep -F 'POINT(1.01 1.04)|103|fixed_end_point' $DUMP

echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
    | grep -F 'LINESTRING(1.01 1.04, 1.01 1.01)|0|added_line'

#-----------------------------------------------------------------------------

set +e

$OSMC --verbose --overwrite --output-database=$DB --output-rings -c 0 $INPUT >$LOG 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'There are 2 nodes where the coastline is not closed.' $LOG
grep 'No polygons created!' $LOG
grep '^There were 1 warnings.$' $LOG
grep '^There were 1 errors.$' $LOG

check_count land_polygons 0;
check_count rings 0;
check_count error_points 2;
check_count error_lines 1;

echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
    | grep -F 'LINESTRING(1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01)|200|not_closed'

#-----------------------------------------------------------------------------
