#ifndef COASTLINE_POLYGONS_HPP
#define COASTLINE_POLYGONS_HPP

/*

  Copyright 2012-2022 Jochen Topf <jochen@topf.org>.

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
  along with OSMCoastline.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <ogr_geometry.h>

#include <memory>
#include <utility>
#include <vector>

class OGRSpatialReference;
class OutputDatabase;

using polygon_vector_type = std::vector<std::unique_ptr<OGRPolygon>>;

/**
 * A collection of land polygons created out of coastlines.
 * Contains operations for SRS transformation, splitting up of large polygons
 * and converting to water polygons.
 */
class CoastlinePolygons {

    /// Output database
    OutputDatabase& m_output;

    /**
     * When splitting polygons we want them to overlap slightly to avoid
     * rendering artefacts. This is the amount each geometry is expanded
     * in each direction.
     */
    double m_expand;

    /**
     * When splitting polygons they are split until they have less than
     * this amount of points in them.
     */
    int m_max_points_in_polygon;

    /**
     * Vector of polygons we want to operate on. This is initialized in
     * the constructor from the polygons created from coastline rings.
     * After that the different methods on this class will convert the
     * polygons and always leave the result in this vector again.
     */
    polygon_vector_type m_polygons;

    /**
     * Max depth after recursive splitting.
     */
    int m_max_split_depth = 0;

    void split_geometry(std::unique_ptr<OGRGeometry>&& geom, int level);
    void split_polygon(std::unique_ptr<OGRPolygon>&& polygon, int level);
    void split_bbox(const OGREnvelope& envelope, polygon_vector_type&& v);

    void add_line_to_output(std::unique_ptr<OGRLineString> line, OGRSpatialReference* srs) const;
    void output_polygon_ring_as_lines(int max_points, const OGRLinearRing* ring) const;

public:

    CoastlinePolygons(polygon_vector_type&& polygons, OutputDatabase& output, double expand, int max_points_in_polygon) :
        m_output(output),
        m_expand(expand),
        m_max_points_in_polygon(max_points_in_polygon),
        m_polygons(std::move(polygons)) {
    }

    /// Number of polygons
    int num_polygons() const noexcept {
        return m_polygons.size();
    }

    polygon_vector_type::const_iterator begin() const noexcept {
        return m_polygons.begin();
    }

    polygon_vector_type::const_iterator end() const noexcept {
        return m_polygons.end();
    }

    /// Turn polygons with wrong winding order around.
    unsigned int fix_direction();

    /// Transform all polygons to output SRS.
    void transform();

    /// Split up all polygons.
    void split();

    /// Check polygons for validity and try to make them valid if needed
    unsigned int check_polygons();

    /// Write all land polygons to the output database.
    void output_land_polygons(bool make_copy);

    /// Write all water polygons to the output database.
    void output_water_polygons();

    /// Write all coastlines to the output database (as lines).
    void output_lines(int max_points) const;

}; // class CoastlinePolygons

#endif // COASTLINE_POLYGONS_HPP
