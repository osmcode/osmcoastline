#ifndef SRS_HPP
#define SRS_HPP

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

#include <ogr_spatialref.h>

class OGRGeometry;
class OGREnvelope;

class SRS {

    /// WGS84 (input) SRS.
    OGRSpatialReference m_srs_wgs84;

    /// Output SRS.
    OGRSpatialReference m_srs_out;

    /**
     * If the output SRS is not WGS84, this contains the transformation
     * object. Otherwise nullptr.
     */
    std::unique_ptr<OGRCoordinateTransformation> m_transform;

public:

    /**
     * This exception is thrown when a transformation to the output SRS fails.
     */
    class TransformationException {};

    SRS() {
        m_srs_wgs84.SetWellKnownGeogCS("WGS84");
    }

    /**
     * Set output SRS to EPGS code. Call this method before using any
     * of the other methods of this object.
     */
    bool set_output(int epsg);

    OGRSpatialReference* wgs84() {
        return &m_srs_wgs84;
    }
    OGRSpatialReference* out()   {
        return &m_srs_out;
    }

    /**
     * Transform geometry to output SRS (if it is not in the output SRS
     * already).
     */
    void transform(OGRGeometry* geometry);

    /**
     * Return max extent for output SRS.
     */
    OGREnvelope max_extent() const;

    /**
     * These values are used to decide which coastline segments are
     * bogus. They are near the antimeridian or southern edge of the
     * map and only there to close the coastline polygons.
     */
    double max_x() const {
        return m_transform ?  20037500.0 :  179.9999;
    }
    double min_x() const {
        return m_transform ? -20037500.0 : -179.9999;
    }
    double min_y() const {
        return m_transform ? -20037400.0 :  -85.049;
    }

};

#endif // SRS_HPP
