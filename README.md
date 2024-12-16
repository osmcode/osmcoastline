
# OSMCoastline

OSMCoastline extracts the coastline data from an OSM planet file and assembles
all the pieces into polygons for use in map renderers etc.

https://osmcode.org/osmcoastline

[![Build Status](https://github.com/osmcode/osmcoastline/actions/workflows/ci.yml/badge.svg)](https://github.com/osmcode/osmcoastline/actions)

## Prerequisites

### Libosmium

    https://github.com/osmcode/libosmium
    https://osmcode.org/libosmium
    At least version 2.16.0 is needed.

### Protozero

    https://github.com/mapbox/protozero
    Debian/Ubuntu: libprotozero-dev
    At least version 1.6.1 is needed.

### LZ4 (optional)

    https://lz4.github.io/lz4/
    Debian/Ubuntu: liblz4-dev

    Only needed for LZ4 PBF compression.

### zlib (for PBF support)

    https://www.zlib.net/
    Debian/Ubuntu: zlib1g-dev

### GDAL (for OGR support)

    https://gdal.org/
    Debian/Ubuntu: libgdal1-dev
    (Must be built with Spatialite and GEOS support which is true for
    Debian/Ubuntu packages. You need GDAL 2 or greater.

### GEOS

    https://trac.osgeo.org/geos/
    Debian/Ubuntu: libgeos-dev

### Sqlite/Spatialite

    https://www.gaia-gis.it/fossil/libspatialite/index
    Debian/Ubuntu: sqlite3, spatialite-bin

### Pandoc (optional, to build documentation)

    https://pandoc.org/
    Debian/Ubuntu: pandoc
    (If pandoc is found by CMake, the manpages will automatically be built.)


## Building

You'll need the prerequisites including `libosmium` installed.

OSMCoastline uses CMake for building:

    mkdir build
    cd build
    cmake ..
    make

Call `make doc` to build the Doxygen API documentation which will be available
in the `doc/html` directory.


## Testing

A few tests are provided that can be run by calling `ctest`.

The tests themselves are written as shell scripts and can be found in the
`test/t` directory. Some test use the `nodegrid2opl` helper program found in
the `src` directory, it has some documentation in the source code.


## The `runtest` script

Run the script `runtest.sh` from the directory you built the program in. It
will read the supplied `testdata.osm` and create output in the `testdata.db`
spatialite database.

It is normal for this program to create errors and warnings, because it is
testing a rather broken input file. You will get messages such as "Closing
ring between node -84 and node -74" and "Warning 1: Self-intersection
at or near point 7.48488 53.8169". At the end it should print:

    There were 35 warnings.
    There were 1 errors.

You can use the supplied `coastline_sqlite.qgs` QGIS project file to open the
output with QGIS.

Call `runtest.sh -v` to run the tests under Valgrind.


## Running

Note that you might want to run `osmcoastline_filter` first, see below under
**Filtering**.

Run: `osmcoastline -o DBFILE PLANET-FILE`

For example: `osmcoastline -o coastline.db planet.osm.pbf`

This will create a spatialite database named `DBFILE` and write several tables
with the output into it.

Use `--verbose` to see whats going on. Start with `--help` to see other
options.


## Output

By default, the output is a spatialite database with the following tables. All
tables are always created but depending on the command line options only some of
them might contain anything.

* `error_lines` Lines that have errors (for instance not closed rings or
  self-intersections).

* `error_points` Problematic points such as intersections.

* `rings` Coastline rings as linestrings. The table is not populated by default,
  because this is only needed for finding errors in the coastline. Use the
  command line option `--output-rings` to populate this table.

* `land_polygons` Finished assembled land polygons. Depending on `--max-points`
  option this will contain complete or split polygons. Only filled if the
  option `--output-polygons=land` (thats the default) or `=both` has been given.

* `water_polygons` Finished assembled water polygons. Only filled if option
  `--output-polygons=water` or `=both` has been given.

* `lines` Coastlines as linestrings. Depending on `--max-points` option this
  will contain complete or split linestrings. Only filled if the option
  `--output-lines` has been given.

By default all output is in WGS84. You can use the option `--srs=3857` to
create output in "Web Mercator". (Other projections are currently not
supported.)



By default geometry indexes are created for all tables. This makes the database
larger, but faster to use. You can use the option `--no-index` to suppress
this, for instance if you never use the data directly anyway but want to
transform it into something else.

Coastlines and polygons are never simplified, but contain the full detail. See
the `simplify_and_split_spatialite` or the `simplify_and_split_postgis`
directories for scripts that help with simplifying and splitting geometries
using Spatialite or PostGIS, respectively.

The database tables `options` and `meta` contain the command line options
used to create the database and some metadata. You can use the script
`osmcoastline_readmeta` to look at them.

By default, OSMCoastline creates a spatialite database. If you need shapefiles
use ogr2ogr to convert the data:

    ogr2ogr -f "ESRI Shapefile" land_polygons.shp coastline.db land_polygons

Alternatively, OSMCoastline aims to support all geospatial data formats as the
output (e.g. Shapefile, by setting GDAL driver as "ESRI Shapefile"). If a data
format other than Spatialite database is selected as the output format, the two
database tables `options` and `meta` will be omitted and geometry indexes will
not be created.


## Steps

OSMCoastline runs in several steps, each can optionally create some output.
In most cases you will only be interested in the end result but preliminary
results are supplied for debugging or other special uses.

**Step 1**: Filter out all nodes and ways tagged `natural=coastline` and all
            nodes needed by those ways. (This can also be done with the
            `osmcoastline_filter` program, see below)

**Step 2**: Assemble all coastline ways into rings. Rings that are not closed
            in the OSM data will be closed depending on the `--close-distance`
            option.

**Step 3**: Assemble polygons from the rings, possibly including holes for
            water areas.

**Step 4**: Split up large polygons into smaller ones. The options
            `--max-points` and `--bbox-overlap` are used here.

**Step 5**: Create water polygons as the "inverse" of the land polygons.

The errors encountered in each step are written to the `error_points` and
`error_lines` tables.


## Options

    -c, --close-distance=DISTANCE

OSMCoastline assembles ways tagged `natural=coastline` into rings. Sometimes
there is a gap in the coastline in the OSM data. OSMCoastline will close this
gap if it is smaller than DISTANCE. Use 0 to disable this feature.

    -b, --bbox-overlap=OVERLAP

Polygons that are too large are split into two halves (recursively if need be).
Where the polygons touch the OVERLAP is added, because two polygons just
touching often lead to rendering problems. The value is given in the units used
for the projection (for WGS84 (4326) this is in degrees, for Mercator (3857)
this is in meters). If this is set too small you might get rendering artefacts
where polygons touch. The larger you set this the larger the output polygons
will be. The best values depend on the map scale or zoom level you are
preparing the data for. Disable the overlap by setting it to 0. Default is
0.0001 for WGS84 and 10 for Mercator.

    -m, --max-points=NUM

Set this to 0 to prevent splitting of large polygons and linestrings. If set to
any other positive integer OSMCoastline will try to split polygons/linestrings
to not have more than this many points. Depending on the overlap defined with
-b and the shape of the polygons it is sometimes not possible to get the
polygons small enough. OSMCoastline will warn you on stderr if this is the
case. Default is 1000.

    -s, --srs=EPSGCODE

Set spatial reference system/projection. Use 4326 for WGS84 or 3857 for "Web
Mercator". If you want to use the data for the usual tiled web maps, 3857 is
probably right. For other uses, especially if you want to re-project to some
other projection, 4326 is probably right. Other projections are currently not
supported. Default is 4326.

    -v, --verbose

Gives you detailed information on what osmcoastline is doing, including timing.

Run `osmcoastline --help` to see all options.


## Return codes

`osmcoastline` uses the following return codes:

    0 - OK
    1 - Warning
    2 - Error
    3 - Fatal error (output file could not be opened etc.)
    4 - Error parsing command line arguments

The difference between warnings and errors is somewhat muddy. Warnings are
geometry problems that have either been fixed automatically or seem to be
small. Errors are larger problems that couldn't be fixed. If there were errors
you probably do not want to use the generated data but fix the OSM data first.
If there were warnings the data might be okay, but there still could be data
missing or geometry problems such as self-intersections in the coastline. But
the classification of problems into warnings and errors is difficult, so to be
on the safe side you might only want to use the data if there are no warnings
and no errors at all.


## Antarctica

OSMCoastline has special code for the coastline of Antarctica. This is the only
coastline that can remain open. The coastline starts somewhere around 180°
East, 77° South and ends around 180° West and 77° South. OSMCoastline will find
those open ends and connect them by adding several "nodes" forming a proper
polygon. Depending on the output projection (EPSG:4326 or EPSG:3857) this
polygon will either extend to the South Pole or to the 85.0511° line.


## Filtering

The program `osmcoastline_filter` can be used to filter from an OSM planet file
all nodes and ways needed for building the coastlines and write them out in
OSM format. The resulting file will be a lot smaller (less than 1%) than the
original planet file, but it contains everything needed to assemble the
coastline polygons.

If you are playing around or want to run `osmcoastline` several times with
different parameters, run `osmcoastline_filter` once first and use its output
as the input for `osmcoastline`.

Run it as follows: `osmcoastline_filter -o OUTFILE.osm.pbf INFILE.osm.pbf`

`osmcoastline_filter` can read PBF and XML files, but write only PBF files. PBF
files are much smaller and faster to read and write.


## Extracts

Generally you can not run OSMCoastline on extracts. OSMCoastline assembles ways
into continuous coastline linestrings and then into land and water polygons.
But if the coastline is not closed, it can't do that. This might work if your
extract only contains an island, but in most cases this will not work.


## License

OSMCoastline is available under the GNU GPL version 3 or later.


## Authors

Jochen Topf (jochen@topf.org)

