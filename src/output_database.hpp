#ifndef OUTPUT_DATABASE_HPP
#define OUTPUT_DATABASE_HPP

/*

  Copyright 2012-2025 Jochen Topf <jochen@topf.org>.

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

#include <osmium/osm/types.hpp>

#include <gdalcpp.hpp>

#include <ctime>
#include <memory>
#include <string>
#include <vector>

class OGRLineString;
class OGRPoint;
class OGRPolygon;
class SRS;

struct Options;
struct Stats;

/**
 * Handle output to a database (via OGR).
 * Several tables/layers are created using the right SRS for the different
 * kinds of data.
 */
class OutputDatabase {

    std::string m_driver;

    bool m_with_index;

    SRS& m_srs;

    gdalcpp::Dataset m_dataset;

    // Any errors in a linestring
    gdalcpp::Layer m_layer_error_points;

    // Any errors in a point
    gdalcpp::Layer m_layer_error_lines;

    // Layer for polygon rings.
    // Will contain polygons without holes, ie. with just an outer ring.
    // Polygon outer rings will be oriented according to usual GIS custom with
    // points going clockwise around the ring, ie "land" is on the right hand
    // side of the border. This is the other way around from how it looks in
    // OSM.
    gdalcpp::Layer m_layer_rings;

    // Completed land polygons.
    gdalcpp::Layer m_layer_land_polygons;

    // Completed water polygons.
    gdalcpp::Layer m_layer_water_polygons;

    // Coastlines generated from completed polygons.
    // Lines contain at most max-points points.
    gdalcpp::Layer m_layer_lines;

    std::vector<std::string> layer_options() const;

    std::vector<std::string> driver_options() const;

public:

    OutputDatabase(const std::string& driver, const std::string& outdb, SRS& srs, bool with_index=false);

    ~OutputDatabase() noexcept = default;

    void add_error_point(std::unique_ptr<OGRPoint>&& point, const char* error, osmium::object_id_type id = 0);
    void add_error_line(std::unique_ptr<OGRLineString>&& linestring, const char* error, osmium::object_id_type id = 0);
    void add_ring(std::unique_ptr<OGRPolygon>&& polygon, osmium::object_id_type osm_id, unsigned int nways, unsigned int npoints, bool fixed);
    void add_land_polygon(std::unique_ptr<OGRPolygon>&& polygon);
    void add_water_polygon(std::unique_ptr<OGRPolygon>&& polygon);
    void add_line(std::unique_ptr<OGRLineString>&& linestring);

    void set_options(const Options& options);
    void set_meta(std::time_t runtime, int memory_usage, const Stats& stats);

    void commit();

}; // class OutputDatabase

#endif // OUTPUT_DATABASE_HPP
