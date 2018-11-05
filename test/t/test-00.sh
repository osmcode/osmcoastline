#!/bin/sh
#-----------------------------------------------------------------------------
#
#  osmcoastline should print usage and help
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

$OSMC >$LOG 2>&1
RC=$?
set -e

test $RC -eq 4

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' $LOG

$OSMC -h >$LOG 2>&1

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' $LOG

$OSMC --help >$LOG 2>&1

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' $LOG

#-----------------------------------------------------------------------------
