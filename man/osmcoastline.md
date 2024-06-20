
# NAME

osmcoastline - extract coastline from OpenStreetMap data


# SYNOPSIS

**osmcoastline** \[*OPTIONS*\] \--output-database=*OUTPUT-DB* *INPUT-FILE*


# DESCRIPTION

**osmcoastline** extracts the coastline data from the *INPUT-FILE*, usually
a planet file (or the output of the **osmcoastline_filter** program, see below)
and assembles all the pieces into polygons for use in map renderers etc.

The output is written to the Spatialite database *OUTPUT-DB*. Depending on the
options it will contain the coastlines in different formats. See the
description of the options below and the README.md for details.


# OPTIONS

-h, \--help
:   Display usage information.

-b, \--bbox-overlap=OVERLAP
:   Polygons that are too large are split into two halves (recursively if need
    be). Where the polygons touch the OVERLAP is added, because two polygons
    just touching often lead to rendering problems. The value is given in the
    units used for the projection (for WGS84 (EPSG: 4326) this is in degrees,
    for Mercator (EPSG: 3857) this is in meters). If this is set too small you
    might get rendering artefacts where polygons touch. The larger you set
    this the larger the output polygons will be. The best values depend on
    the map scale or zoom level you are preparing the data for. Disable the
    overlap by setting it to 0. Default is 0.0001 for WGS84 and 10 for
    Mercator.

-c, \--close-distance=DISTANCE
:   **osmcoastline** assembles ways tagged `natural=coastline` into rings.
    Sometimes there is a gap in the coastline in the OSM data. **osmcoastline**
    will close this gap if it is smaller than DISTANCE. Use 0 to disable this
    feature.

-d, \--debug
:   Enable debugging output.

-e, \--exit-ignore-warnings
:   Return with exit code 0 even if there are warnings.

-f, \--overwrite
:   Overwrite output file if it already exists.

-g, \--gdal-driver=DRIVER
:   Allows user to select any GDAL driver. Only "SQLite", "GPKG" and
    "ESRI Shapefile" GDAL drivers have been tested. The default is "SQLite".

-i, \--no-index
:   Do not create spatial indexes in output db. The default is to create those
    indexes. This makes the database larger, but data access is faster.

-l, \--output-lines
:   Output coastlines as lines to database file.

-m, \--max-points=NUM
:   Set this to 0 to prevent splitting of large polygons and linestrings. If
    set to any other positive integer **osmcoastline** will try to split
    polygons/linestrings to not have more than this many points. Depending on
    the overlap defined with **-b** and the shape of the polygons it is
    sometimes not possible to get the polygons small enough. **osmcoastline**
    will warn you on STDERR if this is the case. Default is 1000.

-o, \--output-database=FILE
:   Spatialite database file for output. This option must be set.

-p, \--output-polygons=land|water|both|none
:   Which polygons to write out (default: land).

-r, \--output-rings
:   Output rings to database file. This is used for debugging.

-s, \--srs=EPSGCODE
:   Set spatial reference system/projection. Use 4326 for WGS84 or 3857 for
    "Web Mercator". If you want to use the data for the usual tiled web
    maps, 3857 is probably right. For other uses, especially if you want to
    re-project to some other projection, 4326 is probably right. Other
    projections are currently not supported. Default is 4326.

-S, \--write-segments=FILENAME
:   Write out all coastline segments to the specified file. Segments are
    connections between two points. The segments are written in an internal
    format intended for use with the **osmcoastline_segments** program
    only. The file includes all segments actually in the OSM data and only
    those. Gaps are (possibly) closed in a later stage of running
    **osmcoastline**, but those closing segments will not be included.

-v, \--verbose
:   Gives you detailed information on what **osmcoastline** is doing,
    including timing.

-V, \--version
:   Display program version and license information.


# NOTES

To speed up processing you might want to run the **osmcoastline_filter**
program first. See its man page for details.


# DIAGNOSTICS

**osmcoastline** exits with exit code

0
  ~ if everything was okay

1
  ~ if there were warnings while processing the coastline (unless -e is set)

2
  ~ if there were errors while processing the coastline

3
  ~ if there was a fatal error when running the program

4
  ~ if there was a problem with the command line arguments.


# EXAMPLES

Run **osmcoastline** on a planet file using default options:

    osmcoastline -o coastline.db planet.osm.pbf

Running **osmcoastline_filter** first:

    osmcoastline_filter -o coastline.osm.pbf planet.osm.pbf
    osmcoastline -o coastline.db coastline.osm.pbf


# SEE ALSO

* `README.md`
* **osmcoastline_filter**(1), **osmcoastline_readmeta**(1),
  **osmcoastline_segments**(1), **osmcoastline_ways**(1)
* [Project page](https://osmcode.org/osmcoastline/)
* [OSMCoastline in OSM wiki](https://wiki.openstreetmap.org/wiki/OSMCoastline)

