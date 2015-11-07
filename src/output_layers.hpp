#ifndef OUTPUT_LAYER_HPP
#define OUTPUT_LAYER_HPP

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

#include <osmium/osm/types.hpp>

#include "srs.hpp"

extern SRS srs;

class OGRLayer;
#if GDAL_VERSION_MAJOR < 2
class OGRDataSource;
#else
class GDALDataset;
#endif
class OGRPoint;
class OGRLineString;
class OGRPolygon;

/**
 * Parent class for all output layers.
 */
class Layer {

protected:

    /// OGRLayer implementing this output layer.
    OGRLayer* m_layer;

    Layer() : m_layer(nullptr) {}

public:

    /// Commit transaction on this layer.
    void commit();

};

/**
 * Layer for any errors in one point.
 */
class LayerErrorPoints : public Layer {

public:

#if GDAL_VERSION_MAJOR < 2
    LayerErrorPoints(OGRDataSource* data_source, const char** options);
#else
    LayerErrorPoints(GDALDataset* data_source, const char** options);
#endif
    void add(OGRPoint* point, const char* error, osmium::object_id_type id);

};

/**
 * Layer for any errors in a linestring.
 */
class LayerErrorLines : public Layer {

public:

#if GDAL_VERSION_MAJOR < 2
    LayerErrorLines(OGRDataSource* data_source, const char** options);
#else
    LayerErrorLines(GDALDataset* data_source, const char** options);
#endif
    void add(OGRLineString* linestring, const char* error, osmium::object_id_type id);

};

/**
 * Layer for polygon rings.
 * Will contain polygons without holes, ie. with just an outer ring.
 * Polygon outer rings will be oriented according to usual GIS custom with
 * points going clockwise around the ring, ie "land" is on the right hand side of
 * the border. This is the other way around from how it looks in OSM.
 */
class LayerRings : public Layer {

public:

#if GDAL_VERSION_MAJOR < 2
    LayerRings(OGRDataSource* data_source, const char** options);
#else
    LayerRings(GDALDataset* data_source, const char** options);
#endif
    void add(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed, LayerErrorPoints* layer_error_points);

};

/**
 * Layer for completed polygons.
 * Polygons can contain inner rings for large water areas such as the Caspian Sea.
 */
class LayerPolygons : public Layer {

    const char* m_name;

public:

#if GDAL_VERSION_MAJOR < 2
    LayerPolygons(OGRDataSource* data_source, const char** options, const char* name);
#else
    LayerPolygons(GDALDataset* data_source, const char** options, const char* name);
#endif
    void add(OGRPolygon* polygon);

};

/**
 * Layer for coastlines generated from completed polygons.
 * Lines containt at most max-points points.
 */
class LayerLines : public Layer {

public:

#if GDAL_VERSION_MAJOR < 2
    LayerLines(OGRDataSource* data_source, const char** options);
#else
    LayerLines(GDALDataset* data_source, const char** options);
#endif
    void add(OGRLineString* lines);

};

#endif // OUTPUT_LAYER_HPP
