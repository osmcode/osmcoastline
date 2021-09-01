#!/bin/sh

readonly SRC_DIR=$1
readonly BIN_DIR=$2
readonly TEST_ID=$3
readonly SRID=$4

readonly OSMC=${BIN_DIR}/src/osmcoastline
readonly INPUT=${BIN_DIR}/test/${TEST_ID}-${SRID}.opl
readonly LOG=${BIN_DIR}/test/${TEST_ID}-${SRID}.log
readonly DB=${BIN_DIR}/test/${TEST_ID}-${SRID}.db
readonly DUMP=${BIN_DIR}/test/${TEST_ID}-${SRID}.dump
readonly SQL="spatialite -bail -batch $DB"

check_count_with_op() {
    test "$(echo "SELECT count(*) FROM $1;" | $SQL)" "$2" "$3"
}

check_count() {
    check_count_with_op "$1" "-eq" "$2"
}

