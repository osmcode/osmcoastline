#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Invalid small "island" with coastline going the wrong direction.
#
#-----------------------------------------------------------------------------

set -x

SRC_DIR=$1
BIN_DIR=$2
TEST_ID=$3

DATA=${SRC_DIR}/test/t/test-${TEST_ID}.osm
OSMC=${BIN_DIR}/src/osmcoastline
LOG=${BIN_DIR}/test/test-${TEST_ID}.log
DB=${BIN_DIR}/test/test-${TEST_ID}.db
SQL="spatialite -bail -batch $DB"

#-----------------------------------------------------------------------------

$OSMC --verbose --overwrite --output-database=$DB $DATA >$LOG 2>&1
RC=$?
set -e

test $RC -eq 1

grep 'Turned 1 polygons around.$' $LOG

grep '^There were 1 warnings.$' $LOG
grep '^There were 0 errors.$' $LOG

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'

#-----------------------------------------------------------------------------
