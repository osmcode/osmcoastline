/*

  Copyright 2012 Jochen Topf <jochen@topf.org>.

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

#include <iostream>
#include <sstream>

#include <ogrsf_frmts.h>

#include "osmcoastline.hpp"
#include "output_database.hpp"
#include "output_layers.hpp"
#include "options.hpp"
#include "stats.hpp"

const char* OutputDatabase::options_with_index[] = { NULL };
const char* OutputDatabase::options_without_index[] = { "SPATIAL_INDEX=no", NULL };

OutputDatabase::OutputDatabase(const std::string& outdb, bool with_index) :
    m_with_index(with_index),
    m_data_source(NULL),
    m_layer_error_points(NULL),
    m_layer_error_lines(NULL),
    m_layer_rings(NULL),
    m_layer_polygons(NULL)
{
    OGRRegisterAll();

    const char* driver_name = "SQLite";
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name);
    if (driver == NULL) {
        std::cerr << driver_name << " driver not available.\n";
        exit(return_code_fatal);
    }

    const char* options[] = { "SPATIALITE=yes", "OGR_SQLITE_SYNCHRONOUS=OFF", NULL };
    m_data_source = driver->CreateDataSource(outdb.c_str(), const_cast<char**>(options));
    if (m_data_source == NULL) {
        std::cerr << "Creation of output file failed.\n";
        exit(return_code_fatal);
    }

    m_layer_error_points = new LayerErrorPoints(m_data_source, layer_options());
    m_layer_error_lines = new LayerErrorLines(m_data_source, layer_options());
    m_layer_rings = new LayerRings(m_data_source, layer_options());
    m_layer_polygons = new LayerPolygons(m_data_source, layer_options());

    exec("CREATE TABLE options (overlap REAL, close_distance REAL, max_points_in_polygons INTEGER, split_large_polygons INTEGER, water INTEGER)");
    exec("CREATE TABLE meta (timestamp TEXT, "
         "runtime INTEGER, "
         "memory_usage INTEGER, "
         "num_ways INTEGER, "
         "num_unconnected_nodes INTEGER, "
         "num_coastline_rings INTEGER, "
         "num_coastline_rings_from_single_way, "
         "num_land_polygons_before_split INTEGER, "
         "num_land_polygons_after_split INTEGER)");
}

void OutputDatabase::set_options(const Options& options) {
    std::ostringstream sql;

    sql << "INSERT INTO options (overlap, close_distance, max_points_in_polygons, split_large_polygons, water) VALUES ("
        << options.bbox_overlap << ", ";

    if (options.close_distance==0) {
        sql << "NULL, ";
    } else {
        sql << options.close_distance << ", ";
    }

    sql << options.max_points_in_polygon << ", "
        << (options.split_large_polygons ? 1 : 0) << ", "
        << (options.water ? 1 : 0)
        << ")";

    exec(sql.str().c_str());
}

void OutputDatabase::set_meta(int runtime, int memory_usage, const Stats& stats) {
    std::ostringstream sql;

    sql << "INSERT INTO meta (timestamp, runtime, "
        << "memory_usage, num_ways, num_unconnected_nodes, num_coastline_rings, num_coastline_rings_from_single_way, "
        << "num_land_polygons_before_split, num_land_polygons_after_split) VALUES (datetime('now'), "
        << runtime << ", "
        << memory_usage << ", "
        << stats.ways << ", "
        << stats.unconnected_nodes << ", "
        << stats.coastline_rings << ", "
        << stats.coastline_rings_from_single_way << ", "
        << stats.land_polygons_before_split << ", "
        << stats.land_polygons_after_split
        << ")";

    exec(sql.str().c_str());
}

OutputDatabase::~OutputDatabase() {
    m_layer_polygons->commit();
    m_layer_rings->commit();
    m_layer_error_lines->commit();
    m_layer_error_points->commit();
    OGRDataSource::DestroyDataSource(m_data_source);
}

void OutputDatabase::add_error_point(OGRPoint* point, const char* error, osm_object_id_t id) {
    m_layer_error_points->add(point, error, id);
}

void OutputDatabase::add_error_line(OGRLineString* linestring, const char* error, osm_object_id_t id) {
    m_layer_error_lines->add(linestring, error, id);
}

void OutputDatabase::add_ring(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed) {
    layer_rings()->add(polygon, id, nways, npoints, fixed, layer_error_points());
}

void OutputDatabase::add_polygon(OGRPolygon* polygon) {
    layer_polygons()->add(polygon);
}

const char** OutputDatabase::layer_options() const {
    return m_with_index ? options_with_index : options_without_index;
}

void OutputDatabase::exec(const char* sql) {
    m_data_source->ReleaseResultSet(m_data_source->ExecuteSQL(sql, NULL, NULL));
}

