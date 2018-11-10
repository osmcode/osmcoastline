#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Invalid way with only two points and invalid way with single point
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

$OSMC --verbose --overwrite --output-database=$DB --output-rings -c 0 $DATA >$LOG 2>&1
RC=$?
set -e

test $RC -eq 2

grep 'There are 2 nodes where the coastline is not closed.' $LOG
grep '^There were 2 warnings.$' $LOG
grep '^There were 1 errors.$' $LOG

check_count land_polygons 0;
check_count rings 0;
check_count error_points 3;
check_count error_lines 1;

echo "SELECT AsText(geometry), osm_id, error FROM error_points;" | $SQL >$DUMP
grep -F 'POINT(1.01 1.01)|100|end_point' $DUMP
grep -F 'POINT(1.04 1.01)|101|end_point' $DUMP
grep -F 'POINT(1.11 1.11)|110|single_point_in_ring' $DUMP

echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
    | grep -F 'LINESTRING(1.04 1.01, 1.01 1.01)|200|not_closed'

#-----------------------------------------------------------------------------
