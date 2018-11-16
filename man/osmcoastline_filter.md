
# NAME

osmcoastline_filter - filter coastline data from OpenStreetMap file


# SYNOPSIS

**osmcoastline_filter** --output=*OUTPUT_FILE* *INPUT-FILE*

**osmcoastline_filter** --help


# DESCRIPTION

**osmcoastline_filter** is used to filter all nodes and ways needed for
building the coastlines from an OSM planet. The data is written to the
output file in PBF format.

This output file will be a lot smaller (less than 1%) than the original planet
file, but it contains everything needed to assemble the coastline and land
or water polygons.

If you are playing around or want to run **osmcoastline** several times with
different parameters, run **osmcoastline_filter** once first and use its output
as the input for **osmcoastline**.

**osmcoastline_filter** can read any file format supported by libosmium (in
particular this is PBF, XML, and OPL files), but write only PBF files.
PBF files are much smaller and faster to read and write than XML files. The
output file will first contain all ways tagged "natural=coastline", then all
nodes used for those ways (and all nodes tagged "natural=coastline"). Having
the ways first and the nodes later in the file is unusual for OSM files, but
the **osmcoastline** and **osmcoastline_ways** programs work fine with it.


# OPTIONS

-h, --help
:   Display usage information

-o, --output=OSMFILE
:   Where to write output (default: none)

-V, --version
:   Display program version and license information.


# EXAMPLES

Run it as follows:

    osmcoastline_filter -o coastline-data.osm.pbf planet.osm.pbf


# SEE ALSO

* **osmcoastline**(1), **osmcoastline_ways**(1)
* [Project page](https://osmcode.org/osmcoastline/)
* [OSMCoastline in OSM wiki](https://wiki.openstreetmap.org/wiki/OSMCoastline)

