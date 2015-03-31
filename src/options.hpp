#ifndef OPTIONS_HPP
#define OPTIONS_HPP

/*

  Copyright 2012-2015 Jochen Topf <jochen@topf.org>.

  This file is part of OSMCoastline.

  OSMCoastline is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OSMCoastline is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OSMCoastline.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string>

enum class output_polygon_type {
    none  = 0,
    land  = 1,
    water = 2,
    both  = 3
};

/**
 * This class encapsulates the command line parsing.
 */
class Options {

public:

    /// Input OSM file name.
    std::string inputfile;

    /// Overlap when splitting polygons.
    double bbox_overlap;

    /**
     * If the distance between two ring-endnodes is smaller than this the ring
     * can be closed there.
     */
    double close_distance;

    /// Attempt to close unclosed rings?
    bool close_rings;

    /// Add spatial index to Spatialite database tables?
    bool create_index;

    /// Show debug output?
    bool debug;

    /// Maximum number of points in polygons.
    int max_points_in_polygon;

    /// Should large polygons be split?
    bool split_large_polygons;

    /// What polygons should be written out?
    output_polygon_type output_polygons;

    /// Output Spatialite database file name.
    std::string output_database;

    /// Should output database be overwritten
    bool overwrite_output;

    /// Should the rings output table be populated?
    bool output_rings;

    /// Should the lines output table be populated?
    bool output_lines;

    /// EPSG code of output SRS.
    int epsg;

    /// Should the coastline be simplified?
    bool simplify;

    /// Tolerance for simplification
    double tolerance;

    /// Verbose output?
    bool verbose;

    Options(int argc, char* argv[]);

private:

    /**
     * Get EPSG code from text. This method knows about a few common cases
     * of specifying WGS84 or the "Google mercator" SRS. More are currently
     * not supported.
     */
    int get_epsg(const char* text);

    void print_help() const;

};

#endif // OPTIONS_HPP
