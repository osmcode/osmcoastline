#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Two valid small "islands" with coastline all around.
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

set -e

$OSMC --verbose --overwrite --output-database=$DB $DATA >$LOG 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' $LOG

grep '^There were 0 warnings.$' $LOG
grep '^There were 0 errors.$' $LOG

check_count land_polygons 2;
check_count error_points 0;
check_count error_lines 0;

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL >$DUMP
grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))' $DUMP
grep -F 'POLYGON((1.01 1.11, 1.01 1.14, 1.04 1.14, 1.04 1.11, 1.01 1.11))' $DUMP

#-----------------------------------------------------------------------------
