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

#include <iostream>

#include <ogr_geometry.h>

#include "coastline_polygons.hpp"
#include "output_database.hpp"

OGRPolygon* CoastlinePolygons::create_rectangular_polygon(double x1, double y1, double x2, double y2, double expand) const {
    x1 -= expand;
    x2 += expand;
    y1 -= expand;
    y2 += expand;

    if (x1 < -180) { x1 = -180; }
    if (x1 >  180) { x1 =  180; }
    if (x2 < -180) { x2 = -180; }
    if (x2 >  180) { x2 =  180; }

    if (y1 <  -90) { y1 =  -90; }
    if (y1 >   90) { y2 =   90; }
    if (y2 <  -90) { y2 =  -90; }
    if (y2 >   90) { y2 =   90; }

    OGRLinearRing* ring = new OGRLinearRing();
    ring->addPoint(x1, y1);
    ring->addPoint(x1, y2);
    ring->addPoint(x2, y2);
    ring->addPoint(x2, y1);
    ring->closeRings();

    OGRPolygon* polygon = new OGRPolygon();
    polygon->addRingDirectly(ring);

    return polygon;
}

unsigned int CoastlinePolygons::fix_direction() {
    unsigned int warnings = 0;

    for (int i=0; i < m_multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(m_multipolygon->getGeometryRef(i));
        if (!p->getExteriorRing()->isClockwise()) {
            p->getExteriorRing()->reverseWindingOrder();
            OGRLineString* l = static_cast<OGRLineString*>(p->getExteriorRing()->clone());
            m_output.add_error(l, "direction");
            warnings++;
        }
    }

    return warnings;
}

void CoastlinePolygons::split(OGRGeometry* g) {
    OGRPolygon* p = static_cast<OGRPolygon*>(g);

    if (p->getExteriorRing()->getNumPoints() <= m_max_points_in_polygon) {
        m_output.add_polygon(p);
    } else {
        OGREnvelope envelope;
        p->getEnvelope(&envelope);

        OGRPolygon* b1;
        OGRPolygon* b2;

        if (envelope.MaxX - envelope.MinX < envelope.MaxY-envelope.MinY) {
            // split vertically
            double MidY = (envelope.MaxY+envelope.MinY) / 2;

            b1 = create_rectangular_polygon(envelope.MinX, envelope.MinY, envelope.MaxX, MidY, m_expand);
            b2 = create_rectangular_polygon(envelope.MinX, MidY, envelope.MaxX, envelope.MaxY, m_expand);
        } else {
            // split horizontally
            double MidX = (envelope.MaxX+envelope.MinX) / 2;

            b1 = create_rectangular_polygon(envelope.MinX, envelope.MinY, MidX, envelope.MaxY, m_expand);
            b2 = create_rectangular_polygon(MidX, envelope.MinY, envelope.MaxX, envelope.MaxY, m_expand);
        }

        OGRGeometry* g1 = p->Intersection(b1);
        OGRGeometry* g2 = p->Intersection(b2);

        if (g1 && (g1->getGeometryType() == wkbPolygon || g1->getGeometryType() == wkbMultiPolygon) &&
            g2 && (g2->getGeometryType() == wkbPolygon || g2->getGeometryType() == wkbMultiPolygon)) {
            if (g1->getGeometryType() == wkbPolygon) {
                split(g1);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g1);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    split(gp);
                }
            }
            if (g2->getGeometryType() == wkbPolygon) {
                split(g2);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g2);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    split(gp);
                }
            }
        } else {
            std::cerr << "g1=" << g1 << " g2=" << g2 << "\n";
            if (g1 != 0) {
                std::cerr << "g1 type=" << g1->getGeometryName() << "\n";
            }
            if (g2 != 0) {
                std::cerr << "g2 type=" << g2->getGeometryName() << "\n";
            }
            m_output.add_polygon(p);
            delete g2;
            delete g1;
        }

        delete b2;
        delete b1;
    }
}

void CoastlinePolygons::output_split_polygons() {
    for (int i=0; i < m_multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(m_multipolygon->getGeometryRef(i));
        if (p->IsValid()) {
            split(p);
        } else {
            OGRPolygon* pb = dynamic_cast<OGRPolygon*>(p->Buffer(0));
            if (pb) {
                std::cerr << "not valid, but buffer ok\n";
                split(pb);
            } else {
                std::cerr << "not valid, and buffer not polygon\n";
            }
        }
    }
}

void CoastlinePolygons::output_complete_polygons() {
    for (int i=0; i < m_multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(m_multipolygon->getGeometryRef(i));
        m_output.add_polygon(p);
    }
}

