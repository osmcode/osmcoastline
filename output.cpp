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

#include "output.hpp"
#include "output_layers.hpp"

Output::Output(const std::string& dirname) :
    m_srs_wgs84(),
    m_layer_error_points(NULL),
    m_layer_error_lines(NULL),
    m_layer_rings(NULL),
    m_layer_polygons(NULL)
{
    OGRRegisterAll();

    m_srs_wgs84.SetWellKnownGeogCS("WGS84");

    const char* driver_name = "ESRI Shapefile";
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name);
    if (driver == NULL) {
        std::cerr << driver_name << " driver not available.\n";
        exit(1);
    }

    const char* options[] = { NULL };
    m_data_source = driver->CreateDataSource(dirname.c_str(), const_cast<char**>(options));
    if (m_data_source == NULL) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }
}

Output::~Output() {
    OGRDataSource::DestroyDataSource(m_data_source);
}

void Output::create_layer_error_points() {
    m_layer_error_points = new LayerErrorPoints(m_data_source, &m_srs_wgs84);
}

void Output::create_layer_error_lines() {
    m_layer_error_lines = new LayerErrorLines(m_data_source, &m_srs_wgs84);
}

void Output::create_layer_rings() {
    m_layer_rings = new LayerRings(m_data_source, &m_srs_wgs84);
}

void Output::create_layer_polygons() {
    m_layer_polygons = new LayerPolygons(m_data_source, &m_srs_wgs84);
}

