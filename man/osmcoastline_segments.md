
# NAME

osmcoastline_segments - analyze coastline changes from segment files


# SYNOPSIS

**osmcoastline_segments** \[*OPTIONS*\] *SEGMENT-FILE1* *SEGMENT-FILE2*


# DESCRIPTION

The **osmcoastline** program can write out all segments of the coastline into a
file (when started with the **-S, --write-segments=FILE** option). This program
can be used to compare two of those segment files in various ways to detect
coastline changes between different runs of the **osmcoastline** program.


# OPTIONS

-h, --help
:   Display usage information.

-d, --dump
:   Dump segment list to stdout in plain text format.

-g, --geom=FILENAME
:   Write segments to geometry file or database using OGR.

-f, --format=FORMAT
:   OGR format for writing out geometries.


# DIAGNOSTICS

**osmcoastline_segments** exits with exit code

0
  ~ if the segment files are the same
1
  ~ if the segment files are different
3
  ~ if there was a fatal error when running the program
4
  ~ if there was a problem with the command line arguments.


# EXAMPLES

Just return 0 or 1 depending on whether the segments are the same in both files:

    osmcoastline_segments old.segments new.segments

Dump the list of removed and added segments to stdout:

    osmcoastline_segments --dump old.segments new.segments

Create a shapefile with the differences:

    osmcoastline_segments --geom=diff.shp old.segments new.segments


# SEE ALSO

* `README.md`
* **osmcoastline**(1)
* [OSMCoastline in OSM wiki](http://wiki.openstreetmap.org/wiki/OSMCoastline)

