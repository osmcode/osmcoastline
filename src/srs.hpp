#ifndef SRS_HPP
#define SRS_HPP

/*

  Copyright 2012-2026 Jochen Topf <jochen@topf.org>.

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

#include <ogr_spatialref.h>

#include <memory>
#include <stdexcept>

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
    class TransformationException : public std::runtime_error {

    public:

        explicit TransformationException(OGRErr error_code) :
            runtime_error("SRS transformation failed - OGRErr=" + std::to_string(error_code)) {
        }

    }; // class TransformationException

    SRS() {
        /* This has to be "CRS84" not "WGS84", because WGS84 has the axis
         * order reversed in GDAL 3 compared to GDAL 2. For CRS84 the axis
         * order is correct in both GDAL versions.
         * See https://gdal.org/tutorials/osr_api_tut.html */
        auto const result = m_srs_wgs84.SetWellKnownGeogCS("CRS84");
        if (result != OGRERR_NONE) {
            throw TransformationException{result};
        }
    }

    /**
     * Set output SRS to EPGS code. Call this method before using any
     * of the other methods of this object.
     */
    bool set_output(int epsg);

    bool is_wgs84() const noexcept {
        return !m_transform;
    }

    OGRSpatialReference* wgs84() {
        return &m_srs_wgs84;
    }

    OGRSpatialReference* out() {
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
    double max_x() const noexcept {
        return is_wgs84() ? 179.9999 : 20037500.0;
    }

    double min_x() const noexcept {
        return is_wgs84() ? -179.9999 : -20037500.0;
    }

    double min_y() const noexcept {
        return is_wgs84() ? -85.049 : -20037400.0;
    }

}; // class SRS

#endif // SRS_HPP
