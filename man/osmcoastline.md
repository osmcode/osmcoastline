
# NAME

osmcoastline - extract coastline from OSM data


# SYNOPSIS

**osmcoastline** \[*OPTIONS*\] --output=*OUTPUT-DB* *INPUT-FILE*


# DESCRIPTION

**osmcoastline** extracts the coastline data from the *INPUT-FILE*, ususally
a planet file (or the output of the **osmcoastline_filter** program, see below)
and assembles all the pieces into polygons for use in map renderers etc.

The output is written to the Spatialite database *OUTPUT-DB*. Depending on the
options it will contains the coastlines in different formats. See the
description of the options below and the README.md for details.


# OPTIONS

-h, --help
:   Display usage information.

-b, --bbox-overlap=OVERLAP
:   Polygons that are too large are split into two halves (recursively if need
    be). Where the polygons touch the OVERLAP is added, because two polygons
    just touching often lead to rendering problems. The value is given in the
    units used for the projection (for WGS84 (4326) this is in degrees, for
    Mercator (3857) this is in meters). If this is set too small you might get
    rendering artefacts where polygons touch. The larger you set this the
    larger the output polygons will be. The best values depend on the map scale
    or zoom level you are preparing the data for. Disable the overlap by
    setting it to 0. Default is 0.0001 for WGS84 and 10 for Mercator.

-c, --close-distance=DISTANCE
:   **osmcoastline** assembles ways tagged `natural=coastline` into rings.
    Sometimes there is a gap in the coastline in the OSM data. OSMCoastline
    will close this gap if it is smaller than DISTANCE. Use 0 to disable this
    feature.

-d, --debug
:   Enable debugging output.

-f, --overwrite
:   Overwrite output file if it already exists.

-i, --no-index
:   Do not create spatial indexes in output db. The default is to create those
    indexes which makes the database larger, but the data is faster to use.

-l, --output-lines
:   Output coastlines as lines to database file.

-m, --max-points=NUM
:   Set this to 0 to prevent splitting of large polygons and linestrings. If
    set to any other positive integer **osmcoastline** will try to split
    polygons/linestrings to not have more than this many points. Depending on
    the overlap defined with **-b** and the shape of the polygons it is
    sometimes not possible to get the polygons small enough. **osmcoastline**
    will warn you on STDERR if this is the case. Default is 1000.

-o, --output=FILE
:   Spatialite database file for output. This option must be set.

-p, --output-polygons=land|water|both|none
:   Which polygons to write out (default: land).

-r, --output-rings
:   Output rings to database file. This is used for debugging purposes.

-s, --srs=EPSGCODE
:   Set spatial reference system/projection. Use 4326 for WGS84 or 3857 for
    "Google Mercator". If you want to use the data for the usual tiled web
    maps, 3857 is probably right. For other uses, especially if you want to
    re-project to some other projection, 4326 is probably right. Other
    projections are curently not supported. Default is 4326.

-v, --verbose
:   Gives you detailed information on what **osmcoastline** is doing,
    including timing.


# NOTES

To speed up processing you might want to run the **osmcoastline_filter**
program first. See its man page for details.


# EXAMPLES

Run **osmcoastline** on a planet file using default options:

    osmcoastline -o coastline.db planet.osm.pbf

Running **osmcoastline_filter** first:

    osmcoastline_filter -o coastline.osm.pbf planet.osm.pbf
    osmcoastline -o coastline.db coastline.osm.pbf


# SEE ALSO

* `README.md`
* **osmcoastline_filter**(1), **osmcoastline_readmeta**(1), **osmcoastline_ways**(1)
* [OSMCoastline in OSM wiki](http://wiki.openstreetmap.org/wiki/OSMCoastline)

