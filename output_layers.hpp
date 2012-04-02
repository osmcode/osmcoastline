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

class OGRLayer;
class OGRDataSource;
class OGRSpatialReference;
class OGRPoint;
class OGRLineString;
class OGRPolygon;
class OGRCoordinateTransformation;

/**
 * Parent class for all output layers.
 */
class Layer {

    /// Coordinate transformation, NULL if output is WGS84.
    OGRCoordinateTransformation* m_transform;

protected:

    /// OGRLayer implementing this output layer.
    OGRLayer* m_layer;

    Layer(OGRCoordinateTransformation* transform) :
        m_transform(transform) {
    }

    /**
     * Transform coordinates to output SRS if they are not already transformed.
     */
    void transform_if_needed(OGRGeometry* geometry) {
        if (!m_transform) { // Output SRS is WGS84, no transformation needed.
            return;
        }

        // Transform if no SRS is set on input geometry or it is set to WGS84.
        if (geometry->getSpatialReference() == NULL || geometry->getSpatialReference()->IsSame(m_transform->GetSourceCS())) {
            if (geometry->transform(m_transform) != OGRERR_NONE) {
                // XXX we should do something more clever here
                std::cerr << "Coordinate transformation failed\n";
                exit(return_code_fatal);
            }
        }
    }

public:

    /// Commit transaction on this layer.
    OGRErr commit();

};

/**
 * Layer for any errors in one point.
 */
class LayerErrorPoints : public Layer {

public:

    LayerErrorPoints(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    void add(OGRPoint* point, const char* error, osm_object_id_t id);

};

/**
 * Layer for any errors in a linestring.
 */
class LayerErrorLines : public Layer {

public:

    LayerErrorLines(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
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

    LayerRings(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    void add(OGRPolygon* polygon, int id, int nways, int npoints, bool fixed, LayerErrorPoints* layer_error_points);

};

/**
 * Layer for completed polygons.
 * Polygons can contain inner rings for large water areas such as the Caspian Sea.
 */
class LayerPolygons : public Layer {

public:

    LayerPolygons(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    void add(OGRPolygon* polygon);

};

#endif // OUTPUT_LAYER_HPP
