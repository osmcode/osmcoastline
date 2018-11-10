#!/bin/sh

readonly SRC_DIR=$1
readonly BIN_DIR=$2
readonly TEST_ID=$3

readonly DATA=${SRC_DIR}/test/t/test-${TEST_ID}.osm
readonly OSMC=${BIN_DIR}/src/osmcoastline
readonly LOG=${BIN_DIR}/test/test-${TEST_ID}.log
readonly DB=${BIN_DIR}/test/test-${TEST_ID}.db
readonly SQL="spatialite -bail -batch $DB"

check_count() {
    test `echo "SELECT count(*) FROM $1;" | $SQL` -eq $2
}

