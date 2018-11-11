#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Invalid small "island" with coastline going the wrong direction.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

$OSMC --verbose --overwrite --output-database=$DB $DATA >$LOG 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Turned 1 polygons around.$' $LOG

grep '^There were 1 warnings.$' $LOG
grep '^There were 0 errors.$' $LOG

check_count land_polygons 1;
check_count error_points 0;
check_count error_lines 1;

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'

echo "SELECT AsText(geometry), osm_id, error FROM error_lines;" | $SQL \
    | grep -F 'LINESTRING(1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01)|0|direction'

#-----------------------------------------------------------------------------
