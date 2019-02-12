/*

  Copyright 2012-2019 Jochen Topf <jochen@topf.org>.

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

#include "coastline_ring.hpp"

#include <osmium/geom/ogr.hpp>
#include <osmium/osm/undirected_segment.hpp>

#include <ogr_geometry.h>

#include <cassert>
#include <iostream>
#include <utility>

void CoastlineRing::setup_locations(locmap_type& locmap) {
    for (auto& wn : m_way_node_list) {
        locmap.insert(std::make_pair(wn.ref(), &(wn.location())));
    }
}

unsigned int CoastlineRing::check_locations(bool output_missing) {
    unsigned int missing_locations = 0;

    for (const auto& wn : m_way_node_list) {
        if (!wn.location()) {
            ++missing_locations;
            if (output_missing) {
                std::cerr << "Missing location of node " << wn.ref() << "\n";
            }
        }
    }

    return missing_locations;
}

void CoastlineRing::add_at_front(const osmium::Way& way) {
    assert(first_node_id() == way.nodes().back().ref());
    m_way_node_list.insert(m_way_node_list.begin(), way.nodes().begin(), way.nodes().end()-1);

    update_ring_id(way.id());
    m_nways++;
}

void CoastlineRing::add_at_end(const osmium::Way& way) {
    assert(last_node_id() == way.nodes().front().ref());
    m_way_node_list.insert(m_way_node_list.end(), way.nodes().begin()+1, way.nodes().end());

    update_ring_id(way.id());
    m_nways++;
}

void CoastlineRing::join(const CoastlineRing& other) {
    assert(last_node_id() == other.first_node_id());
    m_way_node_list.insert(m_way_node_list.end(), other.m_way_node_list.begin()+1, other.m_way_node_list.end());

    update_ring_id(other.ring_id());
    m_nways += other.m_nways;
}

void CoastlineRing::join_over_gap(const CoastlineRing& other) {
    if (last_location() != other.first_location()) {
        m_way_node_list.push_back(other.m_way_node_list.front());
    }

    m_way_node_list.insert(m_way_node_list.end(), other.m_way_node_list.begin()+1, other.m_way_node_list.end());

    update_ring_id(other.ring_id());
    m_nways += other.m_nways;
    m_fixed = true;
}

void CoastlineRing::close_ring() {
    if (first_location() != last_location()) {
        m_way_node_list.push_back(m_way_node_list.front());
    }
    m_fixed = true;
}

void CoastlineRing::close_antarctica_ring(int epsg) {
    const double min = epsg == 4326 ? -90.0 : -85.0511288;

    for (int lat = -78; lat > int(min); --lat) {
        m_way_node_list.emplace_back(0, osmium::Location{-180.0, double(lat)});
    }

    for (int lon = -180; lon < 180; ++lon) {
        m_way_node_list.emplace_back(0, osmium::Location{double(lon), min});
    }

    if (epsg == 3857) {
        m_way_node_list.emplace_back(0, osmium::Location{180.0, min});
    }

    for (auto lat = static_cast<int>(min); lat < -78; ++lat) {
        m_way_node_list.emplace_back(0, osmium::Location{180.0, double(lat)});
    }

    m_way_node_list.push_back(m_way_node_list.front());
    m_fixed = true;
}

std::unique_ptr<OGRPolygon> CoastlineRing::ogr_polygon(osmium::geom::OGRFactory<>& geom_factory, bool reverse) const {
    geom_factory.polygon_start();
    std::size_t num_points = 0;
    if (reverse) {
        num_points = geom_factory.fill_polygon(m_way_node_list.crbegin(), m_way_node_list.crend());
    } else {
        num_points = geom_factory.fill_polygon(m_way_node_list.cbegin(), m_way_node_list.cend());
    }
    return geom_factory.polygon_finish(num_points);
}

std::unique_ptr<OGRLineString> CoastlineRing::ogr_linestring(osmium::geom::OGRFactory<>& geom_factory, bool reverse) const {
    geom_factory.linestring_start();
    std::size_t num_points = 0;
    if (reverse) {
        num_points = geom_factory.fill_linestring(m_way_node_list.crbegin(), m_way_node_list.crend());
    } else {
        num_points = geom_factory.fill_linestring(m_way_node_list.cbegin(), m_way_node_list.cend());
    }
    return geom_factory.linestring_finish(num_points);
}

std::unique_ptr<OGRPoint> CoastlineRing::ogr_first_point() const {
    assert(!m_way_node_list.empty());
    const osmium::NodeRef& node_ref = m_way_node_list.front();
    return std::unique_ptr<OGRPoint>{new OGRPoint{node_ref.lon(), node_ref.lat()}};
}

std::unique_ptr<OGRPoint> CoastlineRing::ogr_last_point() const {
    assert(!m_way_node_list.empty());
    const osmium::NodeRef& node_ref = m_way_node_list.back();
    return std::unique_ptr<OGRPoint>{new OGRPoint{node_ref.lon(), node_ref.lat()}};
}

// Pythagoras doesn't work on a round earth but that is ok here, we only need a
// rough measure anyway
double CoastlineRing::distance_to_start_location(osmium::Location pos) const {
    assert(!m_way_node_list.empty());
    const osmium::Location p = m_way_node_list.front().location();
    return (pos.lon() - p.lon()) * (pos.lon() - p.lon()) +
           (pos.lat() - p.lat()) * (pos.lat() - p.lat());
}

void CoastlineRing::add_segments_to_vector(std::vector<osmium::UndirectedSegment>& segments) const {
    if (m_way_node_list.size() > 1) {
        for (auto it = m_way_node_list.begin(); it != m_way_node_list.end() - 1; ++it) {
            segments.emplace_back(it->location(), (it+1)->location());
        }
    }
}

std::ostream& operator<<(std::ostream& out, CoastlineRing& cp) {
    out << "CoastlineRing(ring_id=" << cp.ring_id()
        << ", nways=" << cp.nways()
        << ", npoints=" << cp.npoints()
        << ", first_node_id=" << cp.first_node_id()
        << ", last_node_id=" << cp.last_node_id();

    if (cp.is_closed()) {
        out << " [CLOSED]";
    }

    out << ")";
    return out;
}

