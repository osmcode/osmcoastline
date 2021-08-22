
# NAME

osmcoastline_readmeta - display metadata from database created by osmcoastline


# SYNOPSIS

**osmcoastline_readmeta** \[*COASTLINE-DB*\]


# OPTIONS

-h, --help
:   Display usage information.

-V, --version
:   Display program version and license information.


# DESCRIPTION

This program displays various meta data from a database created by the
**osmcoastline** program.

If no database name is specified on the command line "testdata.db" is used.

Displayed are the command line options used to create the database, some
statistics about the coastline, the number of warnings and errors detected
and more.

This will only work if the (default) "SQLite" output format has been used
on the **osmcoastline** program.


# SEE ALSO

* **osmcoastline**(1)
* [Project page](https://osmcode.org/osmcoastline/)
* [OSMCoastline in OSM wiki](https://wiki.openstreetmap.org/wiki/OSMCoastline)

