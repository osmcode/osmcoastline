
# NAME

osmcoastline_ways - extract coastline ways from OSM data


# SYNOPSIS

**osmcoastline_ways** *INPUT-FILE* \[*OUTPUT-DB*\]


# OPTIONS

-h, --help
:   Display usage information.

-V, --version
:   Display program version and license information.


# DESCRIPTION

**osmcoastline_ways** extracts coastline ways from OSM data and writes them
into a Spatialite database. The output data is meant for debugging and
statistics. Use the **osmcoastline** program to assemble coastlines for
"real" use.

You have to run **osmcoastline_ways** on the output of the
**osmcoastline_filter** program, otherwise it will not work correctly!

The data is written to the Spatialite database *OUTPUT-DB*. If *OUTPUT-DB* is
not set, the default "coastline-ways.db" is used. A single table "ways" is
written. It contains the IDs of all ways, their geometries, and the contents
of their "name" and "source" tags.

**osmcoastline_ways** outputs the sum of all way lengths.


# SEE ALSO

* **osmcoastline**(1), **osmcoastline_filter**(1)
* [OSMCoastline in OSM wiki](https://wiki.openstreetmap.org/wiki/OSMCoastline)

