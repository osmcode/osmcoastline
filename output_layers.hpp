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

class Layer {

    OGRCoordinateTransformation* m_transform;

protected:

    Layer(OGRCoordinateTransformation* transform) :
        m_transform(transform) {
    }

    void transform_if_needed(OGRGeometry* geometry) {
        if (m_transform && (geometry->getSpatialReference() == NULL || geometry->getSpatialReference()->IsSame(m_transform->GetSourceCS()))) {
            if (geometry->transform(m_transform) != OGRERR_NONE) {
                // XXX we should do something more clever here
                std::cerr << "Coordinate transformation failed\n";
                exit(return_code_fatal);
            }
        }
    }

};

class LayerErrorPoints : Layer {

    OGRLayer* m_layer;

public:

    LayerErrorPoints(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    ~LayerErrorPoints();
    OGRErr commit();
    void add(OGRPoint* point, int id, const char* error);

};

class LayerErrorLines : Layer {

    OGRLayer* m_layer;

public:

    LayerErrorLines(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    ~LayerErrorLines();
    OGRErr commit();
    void add(OGRLineString* linestring, int id, bool is_simple);

};

class LayerRings : Layer {

    OGRLayer* m_layer;

public:

    LayerRings(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    ~LayerRings();
    OGRErr commit();
    void add(OGRPolygon* polygon, int id, int nways, int npoints, LayerErrorPoints* layer_error_points);

};

class LayerPolygons : Layer {

    OGRLayer* m_layer;

public:

    LayerPolygons(OGRDataSource* data_source, OGRCoordinateTransformation* transform, OGRSpatialReference* srs, const char** options);
    ~LayerPolygons();
    OGRErr commit();
    void add(OGRPolygon* polygon, bool clockwise);

};

#endif // OUTPUT_LAYER_HPP
