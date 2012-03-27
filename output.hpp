#ifndef OUTPUT_HPP
#define OUTPUT_HPP

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

#include <ogr_spatialref.h>

class LayerErrorPoints;
class LayerErrorLines;
class LayerRings;
class LayerPolygons;

class OGRDataSource;

/**
 * The Output class encapsulates the OGR output for the OSMCoastline program.
 */
class Output {

    static const char* options_without_index[];
    static const char* options_with_index[];

    bool m_with_index;

    OGRSpatialReference m_srs_wgs84;
    OGRDataSource* m_data_source;

    LayerErrorPoints* m_layer_error_points;
    LayerErrorLines*  m_layer_error_lines;
    LayerRings*       m_layer_rings;
    LayerPolygons*    m_layer_polygons;

    const char** layer_options() const;

public:

    Output(const std::string& outdb, bool with_index=false);

    ~Output();

    void create_layer_error_points();
    void create_layer_error_lines();
    void create_layer_rings();
    void create_layer_polygons();

    LayerErrorPoints* layer_error_points() const { return m_layer_error_points; }
    LayerErrorLines*  layer_error_lines()  const { return m_layer_error_lines; }
    LayerRings*       layer_rings()        const { return m_layer_rings; }
    LayerPolygons*    layer_polygons()     const { return m_layer_polygons; }

};

#endif // OUTPUT_HPP
