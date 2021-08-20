#!/bin/sh

readonly SRC_DIR=$1
readonly BIN_DIR=$2
readonly TEST_ID=$3

readonly OSMC=${BIN_DIR}/src/osmcoastline
readonly INPUT=${BIN_DIR}/test/${TEST_ID}.opl
readonly LOG=${BIN_DIR}/test/${TEST_ID}.log
readonly DB=${BIN_DIR}/test/${TEST_ID}.db
readonly DUMP=${BIN_DIR}/test/${TEST_ID}.dump
readonly SQL="spatialite -bail -batch $DB"

check_count() {
    test `echo "SELECT count(*) FROM $1;" | $SQL` -eq $2
}

check_count_with_op() {
    test `echo "SELECT count(*) FROM $1;" | $SQL` $2 $3
}

