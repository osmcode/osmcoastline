/*

  Copyright 2012-2025 Jochen Topf <jochen@topf.org>.

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

#include "coastline_polygons.hpp"
#include "coastline_ring_collection.hpp"
#include "output_database.hpp"
#include "srs.hpp"

#include <ogr_geometry.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#include <io.h>
#include <BaseTsd.h>
#endif

extern SRS srs;
extern bool debug;

/**
 * If a way is not closed adding it to the coastline collection is a bit
 * complicated.
 * We'll check if there is an existing CoastlineRing that our way connects
 * to and add it to that ring. If there is none, we'll create a new
 * CoastlineRing for it and add that to the collection.
 */
void CoastlineRingCollection::add_partial_ring(const osmium::Way& way) {
    assert(!way.nodes().empty());
    const auto mprev = m_end_nodes.find(way.nodes().front().ref());
    const auto mnext = m_start_nodes.find(way.nodes().back().ref());

    // There is no CoastlineRing yet where this way could fit. So we
    // create one and add it to the collection.
    if (mprev == m_end_nodes.end() &&
        mnext == m_start_nodes.end()) {
        const auto added = m_list.insert(m_list.end(), std::make_shared<CoastlineRing>(way));
        m_start_nodes[way.nodes().front().ref()] = added;
        m_end_nodes[way.nodes().back().ref()] = added;
        return;
    }

    // We found a CoastlineRing where we can add the way at the end.
    if (mprev != m_end_nodes.end()) {
        const auto prev = mprev->second;
        (*prev)->add_at_end(way);
        m_end_nodes.erase(mprev);

        if ((*prev)->is_closed()) {
            const auto found = m_start_nodes.find((*prev)->first_node_id());
            if (found != m_start_nodes.end()) {
                m_start_nodes.erase(found);
            }
            return;
        }

        // We also found a CoastlineRing where we could have added the
        // way at the front. This means that the way together with the
        // ring at front and the ring at back are now a complete ring.
        if (mnext != m_start_nodes.end()) {
            const auto next = mnext->second;
            (*prev)->join(**next);
            m_start_nodes.erase(mnext);
            if ((*prev)->is_closed()) {
                auto x = m_start_nodes.find((*prev)->first_node_id());
                if (x != m_start_nodes.end()) {
                    m_start_nodes.erase(x);
                }
                x = m_end_nodes.find((*prev)->last_node_id());
                if (x != m_end_nodes.end()) {
                    m_end_nodes.erase(x);
                }
            }
            m_list.erase(next);
        }

        m_end_nodes[(*prev)->last_node_id()] = prev;
        return;
    }

    // We found a CoastlineRing where we can add the way at the front.
    if (mnext != m_start_nodes.end()) {
        const auto next = mnext->second;
        (*next)->add_at_front(way);
        m_start_nodes.erase(mnext);
        if ((*next)->is_closed()) {
            const auto found = m_end_nodes.find((*next)->last_node_id());
            if (found != m_end_nodes.end()) {
                m_end_nodes.erase(found);
            }
            return;
        }
        m_start_nodes[(*next)->first_node_id()] = next;
    }
}

void CoastlineRingCollection::setup_locations(locmap_type& locmap) {
    for (const auto& ring : m_list) {
        ring->setup_locations(locmap);
    }
}

unsigned int CoastlineRingCollection::check_locations(bool output_missing) {
    unsigned int missing_locations = 0;

    for (const auto& ring : m_list) {
        missing_locations += ring->check_locations(output_missing);
    }

    return missing_locations;
}

namespace {

bool is_valid_polygon(const OGRGeometry* geometry) {
    if (geometry && geometry->getGeometryType() == wkbPolygon && !geometry->IsEmpty()) {
        const auto *const polygon = static_cast<const OGRPolygon*>(geometry);
        return (polygon->getExteriorRing()->getNumPoints() > 3) && (polygon->getNumInteriorRings() == 0) && geometry->IsValid();
    }
    return false;
}

} // anonymous namespace

std::vector<OGRGeometry*> CoastlineRingCollection::add_polygons_to_vector() {
    std::vector<OGRGeometry*> vector;
    vector.reserve(m_list.size());

    for (const auto& ring : m_list) {
        if (ring->is_closed() && ring->npoints() > 3) { // everything that doesn't match here is bad beyond repair and reported elsewhere
            std::unique_ptr<OGRPolygon> p = ring->ogr_polygon(m_factory, true);
            if (p->IsValid()) {
                p->assignSpatialReference(srs.wgs84());
                vector.push_back(p.release());
            } else {
                std::unique_ptr<OGRGeometry> geom{p->Buffer(0)};
                if (is_valid_polygon(geom.get())) {
                    geom->assignSpatialReference(srs.wgs84());
                    vector.push_back(geom.release());
                } else {
                    std::cerr << "Ignoring invalid polygon geometry (ring_id=" << ring->ring_id() << ").\n";
                }
            }
        }
    }

    return vector;
}

unsigned int CoastlineRingCollection::output_rings(OutputDatabase& output) {
    unsigned int warnings = 0;

    for (const auto& ring : m_list) {
        if (ring->is_closed()) {
            if (ring->npoints() > 3) {
                output.add_ring(ring->ogr_polygon(m_factory, true), ring->ring_id(), ring->nways(), ring->npoints(), ring->is_fixed());
            } else if (ring->npoints() == 1) {
                output.add_error_point(ring->ogr_first_point(), "single_point_in_ring", ring->first_node_id());
                warnings++;
            } else { // ring->npoints() == 2 or 3
                output.add_error_line(ring->ogr_linestring(m_factory, true), "not_a_ring", ring->ring_id());
                output.add_error_point(ring->ogr_first_point(), "not_a_ring", ring->first_node_id());
                output.add_error_point(ring->ogr_last_point(), "not_a_ring", ring->last_node_id());
                warnings++;
            }
        } else {
            output.add_error_line(ring->ogr_linestring(m_factory, true), "not_closed", ring->ring_id());
            output.add_error_point(ring->ogr_first_point(), "end_point", ring->first_node_id());
            output.add_error_point(ring->ogr_last_point(), "end_point", ring->last_node_id());
            warnings++;
        }
    }

    return warnings;
}

namespace {

osmium::Location intersection(const osmium::Segment& s1, const osmium::Segment&s2) {
    if (s1.first()  == s2.first()  ||
        s1.first()  == s2.second() ||
        s1.second() == s2.first()  ||
        s1.second() == s2.second()) {
        return osmium::Location{};
    }

    const double denom = ((s2.second().lat() - s2.first().lat())*(s1.second().lon() - s1.first().lon())) -
                         ((s2.second().lon() - s2.first().lon())*(s1.second().lat() - s1.first().lat()));

    if (denom != 0) {
        const double nume_a = ((s2.second().lon() - s2.first().lon())*(s1.first().lat() - s2.first().lat())) -
                              ((s2.second().lat() - s2.first().lat())*(s1.first().lon() - s2.first().lon()));

        const double nume_b = ((s1.second().lon() - s1.first().lon())*(s1.first().lat() - s2.first().lat())) -
                              ((s1.second().lat() - s1.first().lat())*(s1.first().lon() - s2.first().lon()));

        if ((denom > 0 && nume_a >= 0 && nume_a <= denom && nume_b >= 0 && nume_b <= denom) ||
            (denom < 0 && nume_a <= 0 && nume_a >= denom && nume_b <= 0 && nume_b >= denom)) {
            const double ua = nume_a / denom;
            const double ix = s1.first().lon() + (ua * (s1.second().lon() - s1.first().lon()));
            const double iy = s1.first().lat() + (ua * (s1.second().lat() - s1.first().lat()));
            return {ix, iy};
        }
    }

    return osmium::Location{};
}

bool outside_x_range(const osmium::UndirectedSegment& s1, const osmium::UndirectedSegment& s2) {
    return s1.first().x() > s2.second().x();
}

bool y_range_overlap(const osmium::UndirectedSegment& s1, const osmium::UndirectedSegment& s2) {
    const int tmin = s1.first().y() < s1.second().y() ? s1.first().y( ) : s1.second().y();
    const int tmax = s1.first().y() < s1.second().y() ? s1.second().y() : s1.first().y();
    const int omin = s2.first().y() < s2.second().y() ? s2.first().y()  : s2.second().y();
    const int omax = s2.first().y() < s2.second().y() ? s2.second().y() : s2.first().y();

    return tmin <= omax && omin <= tmax;
}

std::unique_ptr<OGRLineString> create_ogr_linestring(const osmium::Segment& segment) {
    auto line = std::make_unique<OGRLineString>();
    line->setNumPoints(2);
    line->setPoint(0, segment.first().lon(), segment.first().lat());
    line->setPoint(1, segment.second().lon(), segment.second().lat());
    line->setCoordinateDimension(2);

    return line;
}

} // anonymous namespace

/**
 * Checks if there are intersections between any coastline segments.
 * Returns the number of intersections and overlaps.
 */
unsigned int CoastlineRingCollection::check_for_intersections(OutputDatabase& output, int segments_fd) {
    unsigned int overlaps = 0;

    std::vector<osmium::UndirectedSegment> segments;
    if (debug) {
        std::cerr << "Setting up segments...\n";
    }

    for (const auto& ring : m_list) {
        ring->add_segments_to_vector(segments);
    }

    if (debug) {
        std::cerr << "Sorting...\n";
    }

    std::sort(segments.begin(), segments.end());

    if (segments_fd >= 0) {
        if (debug) {
            std::cerr << "Writing segments to file...\n";
        }

        const auto length = segments.size() * sizeof(osmium::UndirectedSegment);
#ifndef _MSC_VER
        if (::write(segments_fd, segments.data(), length) != static_cast<ssize_t>(length)) {
#else
        if (_write(segments_fd, segments.data(), length) != static_cast<int>(length)) {
#endif
            throw std::runtime_error{"Write error"};
        }
    }

    // There can be no intersections if there are less than two segments
    if (segments.size() < 2) {
        return 0;
    }

    if (debug) {
        std::cerr << "Finding intersections...\n";
    }

    std::vector<osmium::Location> intersections;
    for (auto it1 = segments.cbegin(); it1 != segments.cend() - 1; ++it1) {
        const osmium::UndirectedSegment& s1 = *it1;
        for (auto it2 = it1 + 1; it2 != segments.cend(); ++it2) {
            const osmium::UndirectedSegment& s2 = *it2;
            if (s1 == s2) {
                std::unique_ptr<OGRLineString> line = create_ogr_linestring(s1);
                output.add_error_line(std::move(line), "overlap");
                overlaps++;
            } else {
                if (outside_x_range(s2, s1)) {
                    break;
                }
                if (y_range_overlap(s1, s2)) {
                    const auto i = intersection(s1, s2);
                    if (i) {
                        intersections.push_back(i);
                    }
                }
            }
        }
    }

    for (const auto& intersection : intersections) {
        auto point = std::make_unique<OGRPoint>(intersection.lon(), intersection.lat());
        output.add_error_point(std::move(point), "intersection");
    }

    return intersections.size() + overlaps;
}

bool CoastlineRingCollection::close_antarctica_ring(int epsg) {
    for (const auto& ring : m_list) {
        const osmium::Location fpos = ring->first_location();
        const osmium::Location lpos = ring->last_location();
        if (fpos.lon() > 179.99 && lpos.lon() < -179.99 &&
            fpos.lat() <  -77.0 && fpos.lat() >  -78.0 &&
            lpos.lat() <  -77.0 && lpos.lat() >  -78.0) {

            m_end_nodes.erase(ring->last_node_id());
            m_start_nodes.erase(ring->first_node_id());
            ring->close_antarctica_ring(epsg);
            return true;
        }
    }
    return false;
}

void CoastlineRingCollection::close_rings(OutputDatabase& output, bool debug, double max_distance) {
    std::vector<Connection> connections;

    // Create vector with all possible combinations of connections between rings.
    for (const auto& end_node : m_end_nodes) {
        for (const auto& start_node : m_start_nodes) {
            const double distance = (*start_node.second)->distance_to_start_location((*end_node.second)->last_location());
            if (distance < max_distance) {
                connections.emplace_back(distance, end_node.first, start_node.first);
            }
        }
    }

    // Sort vector by distance, shortest at end.
    std::sort(connections.begin(), connections.end(), Connection::sort_by_distance);

    // Go through vector starting with the shortest connections and close rings
    // using the connections in turn.
    while (!connections.empty()) {
        const Connection conn = connections.back();
        connections.pop_back();

        // Invalidate all other connections using one of the same end points.
        connections.erase(remove_if(connections.begin(), connections.end(), conn), connections.end());

        const auto eit = m_end_nodes.find(conn.start_id);
        const auto sit = m_start_nodes.find(conn.end_id);

        if (eit != m_end_nodes.end() && sit != m_start_nodes.end()) {
            if (debug) {
                std::cerr << "Closing ring between node " << conn.end_id << " and node " << conn.start_id << "\n";
            }

            m_fixed_rings++;

            CoastlineRing* e = eit->second->get();
            const CoastlineRing* s = sit->second->get();

            output.add_error_point(e->ogr_last_point(), "fixed_end_point", e->last_node_id());
            output.add_error_point(s->ogr_first_point(), "fixed_end_point", s->first_node_id());

            if (e->last_location() != s->first_location()) {
                auto linestring = std::make_unique<OGRLineString>();
                linestring->addPoint(e->last_location().lon(), e->last_location().lat());
                linestring->addPoint(s->first_location().lon(), s->first_location().lat());
                output.add_error_line(std::move(linestring), "added_line");
            }

            if (e == s) {
                // connect to itself by closing ring
                e->close_ring();

                m_end_nodes.erase(eit);
                m_start_nodes.erase(sit);
            } else {
                // connect to other ring
                e->join_over_gap(*s);

                m_list.erase(sit->second);
                if (e->first_location() == e->last_location()) {
                    output.add_error_point(e->ogr_first_point(), "double_node", e->first_node_id());
                    m_start_nodes.erase(e->first_node_id());
                    m_end_nodes.erase(eit);
                    m_start_nodes.erase(sit);
                    m_end_nodes.erase(e->last_node_id());
                    e->fake_close();
                } else {
                    m_end_nodes[e->last_node_id()] = eit->second;
                    m_end_nodes.erase(eit);
                    m_start_nodes.erase(sit);
                }
            }
        }
    }
}

/**
 * Finds some questionably polygons. This will find
 * a) some polygons touching another polygon in a single point
 * b) holes inside land (those should usually be tagged as water, riverbank, or so, not as coastline)
 *    very large such objects will not be reported, this excludes the Great Lakes etc.
 * c) holes inside holes (those are definitely wrong)
 *
 * Returns the number of warnings.
 */
unsigned int CoastlineRingCollection::output_questionable(const CoastlinePolygons& polygons, OutputDatabase& output) {
    const unsigned int max_nodes_to_be_considered_questionable = 10000;
    unsigned int warnings = 0;

    using lcrp_type = std::pair<osmium::Location, CoastlineRing*>;

    std::vector<lcrp_type> rings;
    rings.reserve(m_list.size());

    // put all rings in a vector...
    for (const auto& ring : m_list) {
        rings.emplace_back(ring->first_location(), ring.get());
    }

    // comparison function that ignores the second part of the pair
    const auto comp = [](const lcrp_type& a, const lcrp_type& b){
        return a.first < b.first;
    };

    // ... and sort it by location of the first node in the ring (this allows binary search in it)
    std::sort(rings.begin(), rings.end(), comp);

    // go through all the polygons that have been created before and mark the outer rings
    for (const auto& polygon : polygons) {
        const OGRLinearRing* exterior_ring = polygon->getExteriorRing();
        assert(exterior_ring);
        const osmium::Location pos{exterior_ring->getX(0), exterior_ring->getY(0)};
        const auto rings_it = lower_bound(rings.begin(), rings.end(), lcrp_type{pos, nullptr}, comp);
        if (rings_it != rings.end()) {
            rings_it->second->set_outer();
        }
    }

    // find all rings not marked as outer and output them to the error_lines table
    for (const auto& ring : m_list) {
        if (!ring->is_outer()) {
            if (ring->is_closed() && ring->npoints() > 3 && ring->npoints() < max_nodes_to_be_considered_questionable) {
                output.add_error_line(ring->ogr_linestring(m_factory, false), "questionable", ring->ring_id());
                warnings++;
            }
        }
    }

    return warnings;
}

