#ifndef COASTLINE_POLYGONS_HPP
#define COASTLINE_POLYGONS_HPP

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

class OGRPolygon;
class OGRMultiPolygon;
class OGREnvelope;
class OutputDatabase;

typedef std::vector<OGRPolygon*> polygon_vector_t;

class CoastlinePolygons {

    OutputDatabase& m_output;
    double m_expand;
    int m_max_points_in_polygon;
    bool m_keep;

    polygon_vector_t* m_polygons;

    OGRPolygon* create_rectangular_polygon(double x1, double y1, double x2, double y2, double expand=0) const;

    void split(OGRGeometry* g);

public:

    CoastlinePolygons(polygon_vector_t* polygons, OutputDatabase& output, double expand, unsigned int max_points_in_polygon) :
        m_output(output),
        m_expand(expand),
        m_max_points_in_polygon(max_points_in_polygon),
        m_keep(false),
        m_polygons(polygons) {
    }

    ~CoastlinePolygons() {
        delete m_polygons;
    }

    /**
     * When polygons have the wrong winding order, this method will fix them.
     */
    unsigned int fix_direction();

    CoastlinePolygons& keep(bool keep) {
        m_keep = keep;
        return *this;
    }

    /**
    * Split up all the polygons and write them out.
    */
    void output_split_polygons();

    /**
    * Write all polygons to the output database.
    */
    void output_polygons();

    void split_bbox(OGREnvelope e, polygon_vector_t* v);
    void output_water_polygons();

    void transform();
};

#endif // COASTLINE_POLYGONS_HPP
