#ifndef OUTPUT_DATABASE_HPP
#define OUTPUT_DATABASE_HPP

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

#include <osmium/osm/types.hpp>
#include <ogr_spatialref.h>

class LayerErrorPoints;
class LayerErrorLines;
class LayerRings;
class LayerPolygons;

class OGRDataSource;
class OGRCoordinateTransformation;

/**
 * Handle output to an sqlite database (via OGR).
 * Several tables/layers are created using the right SRS for the different
 * kinds of data.
 */
class OutputDatabase {

    static const char* options_without_index[];
    static const char* options_with_index[];

    bool m_with_index;

    OGRSpatialReference m_srs_wgs84;
    OGRSpatialReference m_srs_out;
    OGRDataSource* m_data_source;

    // If the output SRS is not WGS84, this contains the transformation object. Otherwise NULL.
    OGRCoordinateTransformation* m_transform;

    LayerErrorPoints* m_layer_error_points;
    LayerErrorLines*  m_layer_error_lines;
    LayerRings*       m_layer_rings;
    LayerPolygons*    m_layer_polygons;

    const char** layer_options() const;

    /// Execute arbitrary SQL command on database
    void exec(const char* sql);

public:

    OutputDatabase(const std::string& outdb, int epsg, bool with_index=false);

    ~OutputDatabase();

    void create_layer_error_points();
    void create_layer_error_lines();
    void create_layer_rings();
    void create_layer_polygons();

    LayerErrorPoints* layer_error_points() const { return m_layer_error_points; }
    LayerErrorLines*  layer_error_lines()  const { return m_layer_error_lines; }
    LayerRings*       layer_rings()        const { return m_layer_rings; }
    LayerPolygons*    layer_polygons()     const { return m_layer_polygons; }

    void add_error(OGRPoint* point, const char* error, osm_object_id_t id);
    void add_error(OGRLineString* linestring, const char* error, osm_object_id_t id=0);

    void set_meta(int runtime, int memory_usage);

};

#endif // OUTPUT_DATABASE_HPP
