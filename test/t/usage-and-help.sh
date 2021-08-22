#!/bin/sh
#-----------------------------------------------------------------------------
#
#  osmcoastline should print usage and help
#
#-----------------------------------------------------------------------------

. $1/test/init.sh

set -x

#-----------------------------------------------------------------------------

"$OSMC" >"$LOG" 2>&1
RC=$?
set -e

test $RC -eq 4

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' "$LOG"

"$OSMC" -h >"$LOG" 2>&1

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' "$LOG"

"$OSMC" --help >"$LOG" 2>&1

grep '^Usage: osmcoastline .OPTIONS. OSMFILE$' "$LOG"

#-----------------------------------------------------------------------------
