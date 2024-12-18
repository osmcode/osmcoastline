/*

  Copyright 2012-2024 Jochen Topf <jochen@topf.org>.

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

#include "srs.hpp"

#include <ogr_core.h>
#include <ogr_geometry.h>

bool SRS::set_output(int epsg) {
    auto const result = m_srs_out.importFromEPSG(epsg);
    if (result != OGRERR_NONE) {
        return false;
    }

    if (epsg != 4326) {
        m_transform = std::unique_ptr<OGRCoordinateTransformation>(OGRCreateCoordinateTransformation(&m_srs_wgs84, &m_srs_out));
        if (!m_transform) {
            return false;
        }
    }

    return true;
}

void SRS::transform(OGRGeometry* geometry) {
    if (!m_transform) { // Output SRS is WGS84, no transformation needed.
        return;
    }

    // Transform if no SRS is set on input geometry or it is set to WGS84.
    const OGRSpatialReference* srs = geometry->getSpatialReference();
    if (srs == nullptr || srs->IsSame(&m_srs_wgs84)) {
        auto const result = geometry->transform(m_transform.get());
        if (result != OGRERR_NONE) {
            throw TransformationException{result};
        }
    }
}

OGREnvelope SRS::max_extent() const {
    OGREnvelope envelope;

    if (is_wgs84()) {
        envelope.MinX = -180.0;
        envelope.MinY =  -90.0;
        envelope.MaxX =  180.0;
        envelope.MaxY =   90.0;
    } else {
        envelope.MinX = -20037508.342789244;
        envelope.MinY = -20037508.342789244;
        envelope.MaxX =  20037508.342789244;
        envelope.MaxY =  20037508.342789244;
    }

    return envelope;
}

