#ifndef OUTPUT_LAYER_HPP
#define OUTPUT_LAYER_HPP

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

#include "srs.hpp"

extern SRS srs;

class OGRLayer;
class OGRDataSource;
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

    Layer() {}

public:

    /// Commit transaction on this layer.
    OGRErr commit();

};

/**
 * Layer for any errors in one point.
 */
class LayerErrorPoints : public Layer {

public:

    LayerErrorPoints(OGRDataSource* data_source, const char** options);
    void add(OGRPoint* point, const char* error, osm_object_id_t id);

};

/**
 * Layer for any errors in a linestring.
 */
class LayerErrorLines : public Layer {

public:

    LayerErrorLines(OGRDataSource* data_source, const char** options);
    void add(OGRLineString* linestring, const char* error, osm_object_id_t id);

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

    LayerRings(OGRDataSource* data_source, const char** options);
    void add(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed, LayerErrorPoints* layer_error_points);

};

/**
 * Layer for completed polygons.
 * Polygons can contain inner rings for large water areas such as the Caspian Sea.
 */
class LayerPolygons : public Layer {

public:

    LayerPolygons(OGRDataSource* data_source, const char** options);
    void add(OGRPolygon* polygon);

};

#endif // OUTPUT_LAYER_HPP
