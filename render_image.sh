#!/bin/sh
#
#  render_image.sh [DBFILE]
#
#  Render polygons in DBFILE using shp2img program from mapserver.
#  Automatically detects SRID and renders accordingly.
#  If no DBFILE is given, testdata.db is used.
#

WIDTH=2048

if [ "x$1" = "x" ]; then
    DBFILE=testdata.db
else
    DBFILE=$1
fi

SRID=`sqlite3 $DBFILE "SELECT srid FROM geometry_columns WHERE f_table_name='polygons'"`

if [ "$SRID" = "4326" ]; then
    # If SRID is WGS84 the image height is half its width
    HEIGHT=`expr $WIDTH / 2`
    EXTENT="-180 -90 180 90"
else
    # If SRID is 3857 (Mercator) the image height is the same as its width
    HEIGHT=$WIDTH
    EXTENT="-20037508.342789244 -20037508.342789244 20037508.342789244 20037508.342789244"
fi

rm -f coastline-mapserver.db
ln -s $DBFILE coastline-mapserver.db
shp2img -m coastline.map -l polygons -i PNG -o polygons.png -s $WIDTH $HEIGHT -e $EXTENT
rm -f coastline-mapserver.db

