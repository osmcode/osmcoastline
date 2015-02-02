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

#include <iostream>
#include <sstream>

#include "ogr_include.hpp"
#include "osmcoastline.hpp"
#include "output_database.hpp"
#include "output_layers.hpp"
#include "options.hpp"
#include "stats.hpp"

const char* OutputDatabase::options_with_index[] = { nullptr };
const char* OutputDatabase::options_without_index[] = { "SPATIAL_INDEX=no", nullptr };

OutputDatabase::OutputDatabase(const std::string& outdb, bool with_index) :
    m_with_index(with_index),
    m_data_source(),
    m_layer_error_points(),
    m_layer_error_lines(),
    m_layer_rings(),
    m_layer_land_polygons(),
    m_layer_water_polygons(),
    m_layer_lines()
{
    OGRRegisterAll();

    const char* driver_name = "SQLite";
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name);
    if (!driver) {
        std::cerr << driver_name << " driver not available.\n";
        exit(return_code_fatal);
    }

    const char* options[] = { "SPATIALITE=yes", "OGR_SQLITE_SYNCHRONOUS=OFF", "INIT_WITH_EPSG=no", nullptr };
    m_data_source = driver->CreateDataSource(outdb.c_str(), const_cast<char**>(options));
    if (!m_data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(return_code_fatal);
    }

    m_layer_error_points = new LayerErrorPoints(m_data_source, layer_options());
    m_layer_error_lines = new LayerErrorLines(m_data_source, layer_options());
    m_layer_rings = new LayerRings(m_data_source, layer_options());
    m_layer_land_polygons = new LayerPolygons(m_data_source, layer_options(), "land_polygons");
    m_layer_water_polygons = new LayerPolygons(m_data_source, layer_options(), "water_polygons");
    m_layer_lines = new LayerLines(m_data_source, layer_options());

    exec("CREATE TABLE options (overlap REAL, close_distance REAL, max_points_in_polygons INTEGER, split_large_polygons INTEGER)");
    exec("CREATE TABLE meta ("
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

void OutputDatabase::set_options(const Options& options) {
    std::ostringstream sql;

    sql << "INSERT INTO options (overlap, close_distance, max_points_in_polygons, split_large_polygons) VALUES ("
        << options.bbox_overlap << ", ";

    if (options.close_distance==0) {
        sql << "NULL, ";
    } else {
        sql << options.close_distance << ", ";
    }

    sql << options.max_points_in_polygon << ", "
        << (options.split_large_polygons ? 1 : 0)
        << ")";

    exec(sql.str().c_str());
}

void OutputDatabase::set_meta(int runtime, int memory_usage, const Stats& stats) {
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

    exec(sql.str().c_str());
}

OutputDatabase::~OutputDatabase() {
    OGRDataSource::DestroyDataSource(m_data_source);
}

void OutputDatabase::commit() {
    m_layer_lines->commit();
    m_layer_water_polygons->commit();
    m_layer_land_polygons->commit();
    m_layer_rings->commit();
    m_layer_error_lines->commit();
    m_layer_error_points->commit();
}

void OutputDatabase::add_error_point(std::unique_ptr<OGRPoint> point, const char* error, osmium::object_id_type id) {
    m_layer_error_points->add(point.release(), error, id);
}

void OutputDatabase::add_error_point(OGRPoint* point, const char* error, osmium::object_id_type id) {
    m_layer_error_points->add(point, error, id);
}

void OutputDatabase::add_error_line(std::unique_ptr<OGRLineString> linestring, const char* error, osmium::object_id_type id) {
    m_layer_error_lines->add(linestring.release(), error, id);
}

void OutputDatabase::add_error_line(OGRLineString* linestring, const char* error, osmium::object_id_type id) {
    m_layer_error_lines->add(linestring, error, id);
}

void OutputDatabase::add_ring(std::unique_ptr<OGRPolygon> polygon, int id, int nways, int npoints, bool fixed) {
    layer_rings()->add(polygon.release(), id, nways, npoints, fixed, layer_error_points());
}

void OutputDatabase::add_ring(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed) {
    layer_rings()->add(polygon, id, nways, npoints, fixed, layer_error_points());
}

void OutputDatabase::add_land_polygon(OGRPolygon* polygon) {
    layer_land_polygons()->add(polygon);
}

void OutputDatabase::add_water_polygon(OGRPolygon* polygon) {
    layer_water_polygons()->add(polygon);
}

void OutputDatabase::add_line(std::unique_ptr<OGRLineString> linestring) {
    layer_lines()->add(linestring.release());
}

const char** OutputDatabase::layer_options() const {
    return m_with_index ? options_with_index : options_without_index;
}

void OutputDatabase::exec(const char* sql) {
    m_data_source->ReleaseResultSet(m_data_source->ExecuteSQL(sql, nullptr, nullptr));
}

