#ifndef OUTPUT_DATABASE_HPP
#define OUTPUT_DATABASE_HPP

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

#include <memory>
#include <string>

#include <osmium/osm/types.hpp>
#include <ogr_spatialref.h>

#include "ogr_include.hpp"

class LayerErrorPoints;
class LayerErrorLines;
class LayerRings;
class LayerPolygons;
class LayerLines;

class Options;
struct Stats;

/**
 * Handle output to an sqlite database (via OGR).
 * Several tables/layers are created using the right SRS for the different
 * kinds of data.
 */
class OutputDatabase {

    static const char* options_without_index[];
    static const char* options_with_index[];

    bool m_with_index;

    struct OGRDataSourceDestroyer {
        void operator()(OGRDataSource* ptr) {
            OGRDataSource::DestroyDataSource(ptr);
        }
    };

    std::unique_ptr<OGRDataSource, OGRDataSourceDestroyer> m_data_source;

    std::unique_ptr<LayerErrorPoints> m_layer_error_points;
    std::unique_ptr<LayerErrorLines>  m_layer_error_lines;
    std::unique_ptr<LayerRings>       m_layer_rings;
    std::unique_ptr<LayerPolygons>    m_layer_land_polygons;
    std::unique_ptr<LayerPolygons>    m_layer_water_polygons;
    std::unique_ptr<LayerLines>       m_layer_lines;

    const char** layer_options() const;

    /// Execute arbitrary SQL command on database
    void exec(const char* sql);

public:

    OutputDatabase(const std::string& outdb, bool with_index=false);

    ~OutputDatabase() = default;

    void create_layer_error_points();
    void create_layer_error_lines();
    void create_layer_rings();
    void create_layer_land_polygons();
    void create_layer_water_polygons();
    void create_layer_lines();

    LayerErrorPoints* layer_error_points()   const { return m_layer_error_points.get(); }
    LayerErrorLines*  layer_error_lines()    const { return m_layer_error_lines.get(); }
    LayerRings*       layer_rings()          const { return m_layer_rings.get(); }
    LayerPolygons*    layer_land_polygons()  const { return m_layer_land_polygons.get(); }
    LayerPolygons*    layer_water_polygons() const { return m_layer_water_polygons.get(); }
    LayerLines*       layer_lines()          const { return m_layer_lines.get(); }

    void add_error_point(std::unique_ptr<OGRPoint> point, const char* error, osmium::object_id_type id=0);
    void add_error_point(OGRPoint* point, const char* error, osmium::object_id_type id=0);
    void add_error_line(std::unique_ptr<OGRLineString> linestring, const char* error, osmium::object_id_type id=0);
    void add_error_line(OGRLineString* linestring, const char* error, osmium::object_id_type id=0);
    void add_ring(std::unique_ptr<OGRPolygon> polygon, int id, int nways, int npoints, bool fixed);
    void add_ring(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed);
    void add_land_polygon(OGRPolygon* polygon);
    void add_water_polygon(OGRPolygon* polygon);
    void add_line(std::unique_ptr<OGRLineString> linestring);

    void set_options(const Options& options);
    void set_meta(int runtime, int memory_usage, const Stats& stats);

    void commit();

};

#endif // OUTPUT_DATABASE_HPP
