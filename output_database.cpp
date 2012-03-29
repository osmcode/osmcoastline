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

#include <ogrsf_frmts.h>

#include "osmcoastline.hpp"
#include "output_database.hpp"
#include "output_layers.hpp"

const char* OutputDatabase::options_with_index[] = { NULL };
const char* OutputDatabase::options_without_index[] = { "SPATIAL_INDEX=no", NULL };

OutputDatabase::OutputDatabase(const std::string& outdb, int epsg, bool with_index) :
    m_with_index(with_index),
    m_srs_wgs84(),
    m_srs_out(),
    m_data_source(NULL),
    m_transform(NULL),
    m_layer_error_points(NULL),
    m_layer_error_lines(NULL),
    m_layer_rings(NULL),
    m_layer_polygons(NULL)
{
    OGRRegisterAll();

    m_srs_wgs84.SetWellKnownGeogCS("WGS84");
    m_srs_out.importFromEPSG(epsg);

    if (epsg != 4326) {
        m_transform = OGRCreateCoordinateTransformation(&m_srs_wgs84, &m_srs_out);
        if (!m_transform) {
            std::cerr << "Can't create coordinate transformation.\n";
            exit(return_code_fatal);  
        }
    }

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

    m_layer_error_points = new LayerErrorPoints(m_data_source, m_transform, &m_srs_out, layer_options());
    m_layer_error_lines = new LayerErrorLines(m_data_source, m_transform, &m_srs_out, layer_options());
    m_layer_rings = new LayerRings(m_data_source, m_transform, &m_srs_out, layer_options());
    m_layer_polygons = new LayerPolygons(m_data_source, m_transform, &m_srs_out, layer_options());
}

OutputDatabase::~OutputDatabase() {
    m_layer_polygons->commit();
    m_layer_rings->commit();
    m_layer_error_lines->commit();
    m_layer_error_points->commit();
    OGRDataSource::DestroyDataSource(m_data_source);
    delete m_transform;
}

void OutputDatabase::add_error(OGRPoint* point, const char* error, osm_object_id_t id) {
    m_layer_error_points->add(point, error, id);
}

void OutputDatabase::add_error(OGRLineString* linestring, const char* error, osm_object_id_t id) {
    m_layer_error_lines->add(linestring, error, id);
}

const char** OutputDatabase::layer_options() const {
    return m_with_index ? options_with_index : options_without_index;
}

