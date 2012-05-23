#!/bin/sh
#
#  osmcoastline_readmeta.sh [COASTLINEDB]
#

if [ "x$1" = "x" ]; then
    DBFILE=testdata.db
else
    DBFILE=$1
fi

if [ ! -e $DBFILE ]; then
    echo "Can't open '$DBFILE'"
    exit 1
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

echo -n "  Spatial reference system (--srid/-s): "
sqlite3 $DBFILE "SELECT CASE srid WHEN 4326 THEN '4326 (WGS84)' WHEN 3857 THEN '3857 (Mercator)' ELSE srid END FROM geometry_columns WHERE f_table_name='land_polygons';"

echo "\nMetadata:\n"

echo -n "  Database created at: "
sqlite3 $DBFILE "SELECT timestamp FROM meta;"

echo -n "  Runtime (minutes): "
sqlite3 $DBFILE "SELECT CAST(round(CAST(runtime AS REAL)/60) AS INT) FROM meta;"

echo -n "  Memory usage (MB): "
sqlite3 $DBFILE "SELECT memory_usage FROM meta;"

echo -n "  Ways tagged natural=coastline: "
sqlite3 $DBFILE "SELECT num_ways FROM meta;"

echo -n "  Number of nodes where coastline is not closed (before fixing): "
sqlite3 $DBFILE "SELECT num_unconnected_nodes FROM meta;"

echo -n "  Coastline rings: "
sqlite3 $DBFILE "SELECT num_rings FROM meta;"

echo -n "  Coastline rings created from a single way: "
sqlite3 $DBFILE "SELECT num_rings_from_single_way FROM meta;"

echo -n "  Coastline rings created from more then one way: "
sqlite3 $DBFILE "SELECT num_rings - num_rings_from_single_way FROM meta;"

echo -n "  Number of rings fixed (closed): "
sqlite3 $DBFILE "SELECT num_rings_fixed FROM meta;"

echo -n "  Number of rings turned around: "
sqlite3 $DBFILE "SELECT num_rings_turned_around FROM meta;"

echo -n "  Number of land polygons before split: "
sqlite3 $DBFILE "SELECT num_land_polygons_before_split FROM meta;"

echo -n "  Number of land polygons after split: "
sqlite3 $DBFILE "SELECT CASE num_land_polygons_after_split WHEN 0 THEN 'NOT SPLIT' ELSE num_land_polygons_after_split END FROM meta;"

echo "\nErrors/warnings (Points):\n"
echo ".width 3 20\nSELECT count(*), error FROM error_points GROUP BY error;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'

echo "\nErrors/warnings (LineStrings):\n"
echo ".width 3 20\nSELECT count(*), error FROM error_lines GROUP BY error;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'

echo "\nOutput:\n"
echo "SELECT count(*), 'land_polygons' FROM land_polygons;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'
echo "SELECT count(*), 'water_polygons' FROM water_polygons;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'
echo "SELECT count(*), 'lines' FROM lines;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'
echo "SELECT count(*), 'rings' FROM rings;" | sqlite3 -column $DBFILE | sed -e 's/^/  /'

echo

