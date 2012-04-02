#!/bin/sh
#
#  osmcoastline_readmeta COASTLINEDB
#

if [ "x$1" = "x" ]; then
    echo "Usage: osmcoastline_readmeta COASTLINEDB"
    exit 1
fi

echo -n "Database created at: "
sqlite3 $1 "SELECT timestamp FROM META;"

echo -n "Runtime (minutes): "
sqlite3 $1 "SELECT CAST(round(CAST(runtime AS REAL)/60) AS INT) FROM META;"

echo -n "Memory usage (MB): "
sqlite3 $1 "SELECT memory_usage FROM META;"

