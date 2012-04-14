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
#include <vector>
#include <cmath>
#include <assert.h>

#include <ogr_geometry.h>

#include "coastline_polygons.hpp"
#include "output_database.hpp"
#include "osmcoastline.hpp"

OGRPolygon* CoastlinePolygons::create_rectangular_polygon(OGRSpatialReference* srs, double x1, double y1, double x2, double y2, double expand) const {
    x1 -= expand;
    x2 += expand;
    y1 -= expand;
    y2 += expand;

    assert(srs != NULL);
    if (srs->IsGeographic()) {
        if (x1 < -180) { x1 = -180; }
        if (x1 >  180) { x1 =  180; }
        if (x2 < -180) { x2 = -180; }
        if (x2 >  180) { x2 =  180; }

        if (y1 <  -90) { y1 =  -90; }
        if (y1 >   90) { y2 =   90; }
        if (y2 <  -90) { y2 =  -90; }
        if (y2 >   90) { y2 =   90; }
    } else {
        if (x1 < -20037508.342789244) { x1 = -20037508.342789244; }
        if (x1 >  20037508.342789244) { x1 =  20037508.342789244; }
        if (x2 < -20037508.342789244) { x2 = -20037508.342789244; }
        if (x2 >  20037508.342789244) { x2 =  20037508.342789244; }

        if (y1 < -20037508.342789244) { y1 = -20037508.342789244; }
        if (y1 >  20037508.342789244) { y2 =  20037508.342789244; }
        if (y2 < -20037508.342789244) { y2 = -20037508.342789244; }
        if (y2 >  20037508.342789244) { y2 =  20037508.342789244; }
    }

    OGRLinearRing* ring = new OGRLinearRing();
    ring->addPoint(x1, y1);
    ring->addPoint(x1, y2);
    ring->addPoint(x2, y2);
    ring->addPoint(x2, y1);
    ring->closeRings();

    OGRPolygon* polygon = new OGRPolygon();
    polygon->addRingDirectly(ring);
    polygon->assignSpatialReference(srs);

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
        if (m_keep) {
            m_polygons.push_back(p);
        } else {
            m_output.add_polygon(p);
        }
    } else {
        OGREnvelope envelope;
        p->getEnvelope(&envelope);
//        std::cerr << "envelope = (" << envelope.MinX << ", " << envelope.MinY << "), (" << envelope.MaxX << ", " << envelope.MaxY << ") num_points=" << p->getExteriorRing()->getNumPoints() << "\n";

        OGRPolygon* b1;
        OGRPolygon* b2;

        if (envelope.MaxX - envelope.MinX < envelope.MaxY-envelope.MinY) {
            // split vertically
            double MidY = (envelope.MaxY+envelope.MinY) / 2;

            b1 = create_rectangular_polygon(p->getSpatialReference(), envelope.MinX, envelope.MinY, envelope.MaxX, MidY, m_expand);
            b2 = create_rectangular_polygon(p->getSpatialReference(), envelope.MinX, MidY, envelope.MaxX, envelope.MaxY, m_expand);
        } else {
            // split horizontally
            double MidX = (envelope.MaxX+envelope.MinX) / 2;

            b1 = create_rectangular_polygon(p->getSpatialReference(), envelope.MinX, envelope.MinY, MidX, envelope.MaxY, m_expand);
            b2 = create_rectangular_polygon(p->getSpatialReference(), MidX, envelope.MinY, envelope.MaxX, envelope.MaxY, m_expand);
        }

        OGRGeometry* g1 = p->Intersection(b1);
        OGRGeometry* g2 = p->Intersection(b2);

        // for some reason there is sometimes no srs on the geometries, so we add them on
        if (g1) {
            g1->assignSpatialReference(p->getSpatialReference());
        }
        if (g2) {
            g2->assignSpatialReference(p->getSpatialReference());
        }

        if (g1 && (g1->getGeometryType() == wkbPolygon || g1->getGeometryType() == wkbMultiPolygon) &&
            g2 && (g2->getGeometryType() == wkbPolygon || g2->getGeometryType() == wkbMultiPolygon)) {
            if (g1->getGeometryType() == wkbPolygon) {
                split(g1);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g1);
                assert(mp->getSpatialReference() != NULL);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    gp->assignSpatialReference(mp->getSpatialReference());
                    split(gp);
                }
            }
            if (g2->getGeometryType() == wkbPolygon) {
                split(g2);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g2);
                assert(mp->getSpatialReference() != NULL);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    gp->assignSpatialReference(mp->getSpatialReference());
                    split(gp);
                }
            }
        } else {
            std::cerr << "g1=" << g1 << " g2=" << g2 << "\n";
            if (g1 != 0) {
                std::cerr << "g1 type=" << g1->getGeometryName() << "\n";
                if (g1->getGeometryType() == wkbGeometryCollection) {
                    std::cerr << "   num=" << static_cast<OGRGeometryCollection*>(g1)->getNumGeometries() << "\n";
                }
            }
            if (g2 != 0) {
                std::cerr << "g2 type=" << g2->getGeometryName() << "\n";
                if (g2->getGeometryType() == wkbGeometryCollection) {
                    std::cerr << "   num=" << static_cast<OGRGeometryCollection*>(g2)->getNumGeometries() << "\n";
                }
            }
            if (m_keep) {
                m_polygons.push_back(p);
            } else {
                m_output.add_polygon(p);
            }
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
                pb->assignSpatialReference(p->getSpatialReference());
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

void CoastlinePolygons::split_bbox(OGREnvelope e, polygon_vector_t* v, OGRSpatialReference* srs) {
//    std::cerr << "envelope = (" << e.MinX << ", " << e.MinY << "), (" << e.MaxX << ", " << e.MaxY << ") v.size()=" << v->size() << "\n";
    if (v->size() < 100) {
        try {
            OGRGeometry* geom = create_rectangular_polygon(srs, e.MinX, e.MinY, e.MaxX, e.MaxY, m_expand);
            assert(geom->getSpatialReference() != NULL);
            for (polygon_vector_t::const_iterator it = v->begin(); it != v->end(); ++it) {
                OGRGeometry* diff = geom->Difference(*it);
                // for some reason there is sometimes no srs on the geometries, so we add them on
                diff->assignSpatialReference(geom->getSpatialReference());
                delete geom;
                geom = diff;
            }
            if (geom) {
                switch (geom->getGeometryType()) {
                    case wkbPolygon:
                        m_output.add_polygon(static_cast<OGRPolygon*>(geom));
                        break;
                    case wkbMultiPolygon:
                        for (int i=static_cast<OGRMultiPolygon*>(geom)->getNumGeometries() - 1; i >= 0; --i) {
                            OGRPolygon* p = static_cast<OGRPolygon*>(static_cast<OGRMultiPolygon*>(geom)->getGeometryRef(i));
                            p->assignSpatialReference(geom->getSpatialReference());
                            static_cast<OGRMultiPolygon*>(geom)->removeGeometry(i, FALSE);
                            m_output.add_polygon(p);
                        }
                        delete geom;
                        break;
                    case wkbGeometryCollection:
                        // XXX
                        break;
                    default:
                        std::cerr << "IGNORING envelope = (" << e.MinX << ", " << e.MinY << "), (" << e.MaxX << ", " << e.MaxY << ") type=" << geom->getGeometryName() << "\n";
                        // ignore XXX
                        break;
                }
            }
        } catch(...) {
            std::cerr << "ignoring exception\n";
        }
        delete v;
    } else {

        OGREnvelope e1;
        OGREnvelope e2;

        if (e.MaxX - e.MinX < e.MaxY-e.MinY) {
            // split vertically
            double MidY = (e.MaxY+e.MinY) / 2;

            e1.MinX = e.MinX;
            e1.MinY = e.MinY;
            e1.MaxX = e.MaxX;
            e1.MaxY = MidY;

            e2.MinX = e.MinX;
            e2.MinY = MidY;
            e2.MaxX = e.MaxX;
            e2.MaxY = e.MaxY;

        } else {
            // split horizontally
            double MidX = (e.MaxX+e.MinX) / 2;

            e1.MinX = e.MinX;
            e1.MinY = e.MinY;
            e1.MaxX = MidX;
            e1.MaxY = e.MaxY;

            e2.MinX = MidX;
            e2.MinY = e.MinY;
            e2.MaxX = e.MaxX;
            e2.MaxY = e.MaxY;

        }

        polygon_vector_t* v1 = new polygon_vector_t;
        polygon_vector_t* v2 = new polygon_vector_t;
        for (polygon_vector_t::const_iterator it = v->begin(); it != v->end(); ++it) {

            /* You might think re-computing the envelope of all those polygons
            again and again might take a lot of time, but I benchmarked it and
            it has no measurable impact. */
            OGREnvelope e;
            (*it)->getEnvelope(&e);
            if (e1.Intersects(e)) {
                v1->push_back(*it);
            }

            if (e2.Intersects(e)) {
                v2->push_back(*it);
            }
        }
        delete v;
        split_bbox(e1, v1, srs);
        split_bbox(e2, v2, srs);
    }
}


void CoastlinePolygons::output_water_polygons() {
    polygon_vector_t* v = new polygon_vector_t;
    for (std::vector<OGRPolygon*>::iterator it = m_polygons.begin(); it != m_polygons.end(); ++it) {
        OGRPolygon* p = *it;
        if (p->IsValid()) {
            v->push_back(p);
        } else {
            std::cerr << "invalid, using buffer(0)\n";
            v->push_back(static_cast<OGRPolygon*>(p->Buffer(0)));
        }
    }
    OGREnvelope e;

    if (m_polygons[0]->getSpatialReference()->IsGeographic()) {
        e.MinX = -180;
        e.MinY =  -90;
        e.MaxX =  180;
        e.MaxY =   90;
    } else {
        e.MinX = -20037508.342789244;
        e.MinY = -20037508.342789244;
        e.MaxX =  20037508.342789244;
        e.MaxY =  20037508.342789244;
    }
    split_bbox(e, v, m_polygons[0]->getSpatialReference());
}

void CoastlinePolygons::transform(OGRCoordinateTransformation* transform) {
    if (m_multipolygon->transform(transform) != OGRERR_NONE) {
        // XXX we should do something more clever here
        std::cerr << "Coordinate transformation failed\n";
        exit(return_code_fatal);
    }
}

