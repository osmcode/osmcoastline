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

#include "options.hpp"
#include "output_database.hpp"
#include "srs.hpp"
#include "stats.hpp"

#include <gdal_version.h>
#include <geos_c.h>
#include <ogr_core.h>
#include <ogr_geometry.h>

#include <cstddef>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

OutputDatabase::OutputDatabase(const std::string& driver, const std::string& outdb, SRS& srs, bool with_index) :
    m_driver(driver),
    m_with_index(with_index),
    m_srs(srs),
    m_dataset(driver, outdb, gdalcpp::SRS(*srs.out()), driver_options()),
    m_layer_error_points(m_dataset, "error_points", wkbPoint, layer_options()),
    m_layer_error_lines(m_dataset, "error_lines", wkbLineString, layer_options()),
    m_layer_rings(m_dataset, "rings", wkbPolygon, layer_options()),
    m_layer_land_polygons(m_dataset, "land_polygons", wkbPolygon, layer_options()),
    m_layer_water_polygons(m_dataset, "water_polygons", wkbPolygon, layer_options()),
    m_layer_lines(m_dataset, "lines", wkbLineString, layer_options()) {

    m_layer_error_points.add_field("osm_id", OFTString, 10);
    m_layer_error_points.add_field("error", OFTString, 16);

    m_layer_error_lines.add_field("osm_id", OFTString, 10);
    m_layer_error_lines.add_field("error", OFTString, 16);

    m_layer_rings.add_field("osm_id",  OFTString, 10);
    m_layer_rings.add_field("nways",   OFTInteger, 6);
    m_layer_rings.add_field("npoints", OFTInteger, 8);
    m_layer_rings.add_field("fixed",   OFTInteger, 1);
    m_layer_rings.add_field("land",    OFTInteger, 1);
    m_layer_rings.add_field("valid",   OFTInteger, 1);

    if (m_driver == "SQLite") {
        m_dataset.exec("CREATE TABLE options (overlap REAL, close_distance REAL, max_points_in_polygons INTEGER, split_large_polygons INTEGER)");
        m_dataset.exec("CREATE TABLE meta ("
            "timestamp                      TEXT, "
            "runtime                        INTEGER, "
            "memory_usage                   INTEGER, "
            "num_ways                       INTEGER, "
            "num_unconnected_nodes          INTEGER, "
            "num_rings                      INTEGER, "
            "num_rings_from_single_way      INTEGER, "
            "num_rings_fixed                INTEGER, "
            "num_rings_turned_around        INTEGER, "
            "num_land_polygons_before_split INTEGER, "
            "num_land_polygons_after_split  INTEGER)");
    }

    m_dataset.start_transaction();
    m_layer_rings.start_transaction();
    m_layer_land_polygons.start_transaction();
    m_layer_water_polygons.start_transaction();
    m_layer_lines.start_transaction();
    m_layer_error_points.start_transaction();
    m_layer_error_lines.start_transaction();
}

void OutputDatabase::set_options(const Options& options) {
    if (m_driver != "SQLite") {
        return;
    }

    std::ostringstream sql;

    sql << "INSERT INTO options (overlap, close_distance, max_points_in_polygons, split_large_polygons) VALUES ("
        << options.bbox_overlap << ", ";

    if (options.close_distance == 0) {
        sql << "NULL, ";
    } else {
        sql << options.close_distance << ", ";
    }

    sql << options.max_points_in_polygon << ", "
        << (options.split_large_polygons ? 1 : 0)
        << ")";

    m_dataset.exec(sql.str());
}

void OutputDatabase::set_meta(std::time_t runtime, int memory_usage, const Stats& stats) {
    if (m_driver != "SQLite") {
        return;
    }

    std::ostringstream sql;

    sql << "INSERT INTO meta (timestamp, runtime, memory_usage, "
        << "num_ways, num_unconnected_nodes, num_rings, num_rings_from_single_way, num_rings_fixed, num_rings_turned_around, "
        << "num_land_polygons_before_split, num_land_polygons_after_split) VALUES (datetime('now'), "
        << runtime << ", "
        << memory_usage << ", "
        << stats.ways << ", "
        << stats.unconnected_nodes << ", "
        << stats.rings << ", "
        << stats.rings_from_single_way << ", "
        << stats.rings_fixed << ", "
        << stats.rings_turned_around << ", "
        << stats.land_polygons_before_split << ", "
        << stats.land_polygons_after_split
        << ")";

    m_dataset.exec(sql.str());
}

void OutputDatabase::commit() {
    m_layer_error_lines.commit_transaction();
    m_layer_error_points.commit_transaction();
    m_layer_lines.commit_transaction();
    m_layer_water_polygons.commit_transaction();
    m_layer_land_polygons.commit_transaction();
    m_layer_rings.commit_transaction();
    m_dataset.commit_transaction();
}

void OutputDatabase::add_error_point(std::unique_ptr<OGRPoint>&& point, const char* error, osmium::object_id_type id) {
    m_srs.transform(point.get());
    gdalcpp::Feature feature{m_layer_error_points, std::move(point)};
    feature.set_field("osm_id", std::to_string(id).c_str());
    feature.set_field("error", error);
    feature.add_to_layer();
}

void OutputDatabase::add_error_line(std::unique_ptr<OGRLineString>&& linestring, const char* error, osmium::object_id_type id) {
    m_srs.transform(linestring.get());
    gdalcpp::Feature feature{m_layer_error_lines, std::move(linestring)};
    feature.set_field("osm_id", std::to_string(id).c_str());
    feature.set_field("error", error);
    feature.add_to_layer();
}

void OutputDatabase::add_ring(std::unique_ptr<OGRPolygon>&& polygon, osmium::object_id_type osm_id, unsigned int nways, unsigned int npoints, bool fixed) {
    m_srs.transform(polygon.get());

    const bool land = polygon->getExteriorRing()->isClockwise();
    const bool valid = polygon->IsValid();

    if (!valid) {
        /*
           When the polygon is invalid we find out what and where the problem is.
           This code is a bit strange because older versions of the GEOS library
           only export this information as a string. We parse the reason and
           point coordinates (of a self-intersection-point for instance) from
           this string and create a point in the error layer for it.

           The exportToGEOS() method on OGR geometries is not documented. Let's
           hope that it will always be available. We use the GEOSisValidReason()
           function from the GEOS C interface to get to the reason.
        */
        GEOSContextHandle_t contextHandle = OGRGeometry::createGEOSContext();
        char* r = GEOSisValidReason(polygon->exportToGEOS(contextHandle));
        std::string reason = r ? r : "";
        OGRGeometry::freeGEOSContext(contextHandle);

        if (!reason.empty()) {
            const std::size_t left_bracket = reason.find('[');
            const std::size_t right_bracket = reason.find(']');

            std::istringstream iss{reason.substr(left_bracket+1, right_bracket-left_bracket-1), std::istringstream::in};
            double x = NAN;
            double y = NAN;
            iss >> x;
            iss >> y;
            reason.resize(left_bracket);

            auto point = std::make_unique<OGRPoint>();
            point->assignSpatialReference(polygon->getSpatialReference());
            point->setX(x);
            point->setY(y);

            if (reason == "Self-intersection") {
                reason = "self_intersection";
            }
            add_error_point(std::move(point), reason.c_str(), osm_id);
        } else {
            std::cerr << "Did not get reason from GEOS why polygon " << osm_id << " is invalid. Could not write info to error points layer\n";
        }
    }

    gdalcpp::Feature feature{m_layer_rings, std::move(polygon)};
    feature.set_field("osm_id", static_cast<int>(osm_id));
    feature.set_field("nways", static_cast<int>(nways));
    feature.set_field("npoints", static_cast<int>(npoints));
    feature.set_field("fixed", fixed);
    feature.set_field("land", land);
    feature.set_field("valid", valid);
    feature.add_to_layer();
}

void OutputDatabase::add_land_polygon(std::unique_ptr<OGRPolygon>&& polygon) {
    m_srs.transform(polygon.get());
    gdalcpp::Feature feature{m_layer_land_polygons, std::move(polygon)};
    feature.add_to_layer();
}

void OutputDatabase::add_water_polygon(std::unique_ptr<OGRPolygon>&& polygon) {
    m_srs.transform(polygon.get());
    gdalcpp::Feature feature{m_layer_water_polygons, std::move(polygon)};
    feature.add_to_layer();
}

void OutputDatabase::add_line(std::unique_ptr<OGRLineString>&& linestring) {
    m_srs.transform(linestring.get());
    gdalcpp::Feature feature{m_layer_lines, std::move(linestring)};
    feature.add_to_layer();
}

std::vector<std::string> OutputDatabase::layer_options() const {
    std::vector<std::string> options;
    if (!m_with_index && (m_driver == "SQLite" || m_driver == "GPKG")) {
        options.emplace_back("SPATIAL_INDEX=no");
    }
    return options;
}

std::vector<std::string> OutputDatabase::driver_options() const {
    std::vector<std::string> options;
    if (m_driver == "SQLite") {
        options.emplace_back("SPATIALITE=TRUE");
        options.emplace_back("INIT_WITH_EPSG=no");
    }
    return options;
}
