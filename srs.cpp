/*

  Copyright 2012-2014 Jochen Topf <jochen@topf.org>.

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

#include <ogr_geometry.h>

#include "srs.hpp"

SRS::SRS() :
        m_srs_wgs84(),
        m_srs_out(),
        m_transform(NULL)
{
    m_srs_wgs84.SetWellKnownGeogCS("WGS84");
}

SRS::~SRS() {
    delete m_transform;
}

bool SRS::set_output(int epsg) {
    m_srs_out.importFromEPSG(epsg);

    if (epsg != 4326) {
        m_transform = OGRCreateCoordinateTransformation(&m_srs_wgs84, &m_srs_out);
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
    OGRSpatialReference* srs = geometry->getSpatialReference();
    if (srs == NULL || srs->IsSame(&m_srs_wgs84)) {
        if (geometry->transform(m_transform) != OGRERR_NONE) {
            throw TransformationException();
        }
    }
}

OGREnvelope SRS::max_extent() const {
    OGREnvelope envelope;

    if (m_transform) {
        envelope.MinX = -20037508.342789244;
        envelope.MinY = -20037508.342789244;
        envelope.MaxX =  20037508.342789244;
        envelope.MaxY =  20037508.342789244;
    } else {
        envelope.MinX = -180;
        envelope.MinY =  -90;
        envelope.MaxX =  180;
        envelope.MaxY =   90;
    }

    return envelope;
}

