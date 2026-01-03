#ifndef OPTIONS_HPP
#define OPTIONS_HPP

/*

  Copyright 2012-2026 Jochen Topf <jochen@topf.org>.

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
  along with OSMCoastline.  If not, see <https://www.gnu.org/licenses/>.

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
struct Options {

    /// Input OSM file name.
    std::string inputfile;

    /// Overlap when splitting polygons.
    double bbox_overlap = -1.0;

    /**
     * If the distance between two ring-endnodes is smaller than this the ring
     * can be closed there.
     */
    double close_distance = 1.0;

    /// Add spatial index to Spatialite database tables?
    bool create_index = true;

    /// Show debug output?
    bool debug = false;

    /// GDAL driver.
    std::string driver = "SQLite";

    /// Maximum number of points in polygons.
    int max_points_in_polygon = 1000;

    /// Should large polygons be split?
    bool split_large_polygons = true;

    /// What polygons should be written out?
    output_polygon_type output_polygons = output_polygon_type::land;

    /// Output database file name.
    std::string output_database;

    /// Should output database be overwritten
    bool overwrite_output = false;

    /// Should the rings output table be populated?
    bool output_rings = false;

    /// Should the lines output table be populated?
    bool output_lines = false;

    /// EPSG code of output SRS.
    int epsg = 4326;

    /// Should the coastline be simplified?
    bool simplify = false;

    /// Tolerance for simplification
    double tolerance = 0.0;

    /// Verbose output?
    bool verbose = false;

    /// Exit program with code 0 even if there are warnings?
    bool exit_ignore_warnings = false;

    /// Name of optional segment file
    std::string segmentfile;

    int parse(int argc, char* argv[]);

}; // struct Options

#endif // OPTIONS_HPP
