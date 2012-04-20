#!/bin/sh
#
#  osmcoastline_readmeta [COASTLINEDB]
#

if [ "x$1" = "x" ]; then
    DBFILE=testdata.db
else
    DBFILE=$1
fi

echo "Options used to create this data:\n"

echo -n "  Overlap (--bbox-overlap/-b): "
sqlite3 $DBFILE "SELECT overlap FROM options;"

echo -n "  Close gaps in coastline smaller than (--close-distance/-c): "
sqlite3 $DBFILE "SELECT close_distance FROM options;"

echo -n "  Max points in polygons (--max-points/-m): "
sqlite3 $DBFILE "SELECT max_points_in_polygons FROM options;"

echo -n "  Split large polygons: "
sqlite3 $DBFILE "SELECT CASE split_large_polygons WHEN 0 THEN 'no' ELSE 'yes' END FROM options;"

echo -n "  Create water polygons (--water/-w): "
sqlite3 $DBFILE "SELECT CASE water WHEN 0 THEN 'no' ELSE 'yes' END FROM options;"

echo "\nMetadata:\n"

echo -n "  Database created at: "
sqlite3 $DBFILE "SELECT timestamp FROM meta;"

echo -n "  Runtime (minutes): "
sqlite3 $DBFILE "SELECT CAST(round(CAST(runtime AS REAL)/60) AS INT) FROM meta;"

echo -n "  Memory usage (MB): "
sqlite3 $DBFILE "SELECT memory_usage FROM meta;"

echo -n "  Number of land polygons before split: "
sqlite3 $DBFILE "SELECT num_land_polygons_before_split FROM meta;"

echo -n "  Number of land polygons after split (0 if not split): "
sqlite3 $DBFILE "SELECT num_land_polygons_after_split FROM meta;"

