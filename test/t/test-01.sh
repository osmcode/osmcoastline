#!/bin/sh
#-----------------------------------------------------------------------------
#
#  Valid small "island" with coastline all around.
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

set -e

$OSMC --verbose --overwrite --output-database=$DB $DATA >$LOG 2>&1

test $? -eq 0

grep 'Turned 0 polygons around.$' $LOG

grep '^There were 0 warnings.$' $LOG
grep '^There were 0 errors.$' $LOG

echo "SELECT AsText(geometry) FROM land_polygons;" | $SQL \
    | grep -F 'POLYGON((1.01 1.01, 1.01 1.04, 1.04 1.04, 1.04 1.01, 1.01 1.01))'

#-----------------------------------------------------------------------------
