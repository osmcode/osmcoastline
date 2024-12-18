#ifndef COASTLINE_RING_HPP
#define COASTLINE_RING_HPP

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

#include <osmium/geom/ogr.hpp>
#include <osmium/osm/undirected_segment.hpp>
#include <osmium/osm/way.hpp>

#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <vector>

class OGRPoint;
class OGRLineString;
class OGRPolygon;

using locmap_type = std::multimap<osmium::object_id_type, osmium::Location*>;

/**
 * The CoastlineRing class models a (possibly unfinished) ring of
 * coastline, ie. a closed list of points.
 *
 * CoastlineRing objects are created from a way. If the way is
 * already closed we are done. If not, more ways will be added
 * later until the ring is closed.
 *
 * The CoastlineRing keeps track of all the node IDs needed for
 * the ring.
 *
 * To get a unique ID for the coastline ring, the minimum way
 * ID is also kept.
 *
 * By definition coastlines in OSM are tagged as natural=coastline
 * and the land is always to the *left* of the way, the water to
 * the right. So a ring around an island is going counter-clockwise.
 * In normal GIS use, outer rings of polygons are often expected
 * to be clockwise, and inner rings are count-clockwise. So, if we
 * are describing land polygons, the OSM convention is just the
 * opposite of the GIS convention. Because of this the methods
 * on CoastlineRing that create OGR geometries will actually turn
 * the rings around.
 */
class CoastlineRing {

    std::vector<osmium::NodeRef> m_way_node_list{};

    /**
     * Smallest ID of all the ways making up the ring. Can be used as somewhat
     * stable unique ID for the ring.
     */
    osmium::object_id_type m_ring_id;

    /**
     * The number of ways making up this ring. This is not actually needed for
     * anything, but kept to create statistics.
     */
    unsigned int m_nways = 1;

    /// Ring was fixed because of missing/wrong OSM data.
    bool m_fixed = false;

    /// Is this an outer ring?
    bool m_outer = false;

public:

    /**
     * Create CoastlineRing from a way.
     */
    explicit CoastlineRing(const osmium::Way& way) :
        m_ring_id(way.id()) {
        assert(!way.nodes().empty());
        m_way_node_list.reserve(way.is_closed() ? way.nodes().size() : 1000);
        m_way_node_list.insert(m_way_node_list.begin(), way.nodes().begin(), way.nodes().end());
    }

    bool is_outer() const noexcept {
        return m_outer;
    }

    void set_outer() noexcept {
        m_outer = true;
    }

    /// ID of first node in the ring.
    osmium::object_id_type first_node_id() const noexcept {
        assert(!m_way_node_list.empty());
        return m_way_node_list.front().ref();
    }

    /// ID of last node in the ring.
    osmium::object_id_type last_node_id() const noexcept {
        assert(!m_way_node_list.empty());
        return m_way_node_list.back().ref();
    }

    /// Location of the first node in the ring.
    osmium::Location first_location() const noexcept {
        assert(!m_way_node_list.empty());
        return m_way_node_list.front().location();
    }

    /// Location of the last node in the ring.
    osmium::Location last_location() const noexcept {
        assert(!m_way_node_list.empty());
        return m_way_node_list.back().location();
    }

    /// Return ID of this ring (defined as smallest ID of the ways making up the ring).
    osmium::object_id_type ring_id() const noexcept {
        return m_ring_id;
    }

    /**
     * Set ring ID. The ring will only get the new ID if it is smaller than the
     * old one.
     */
    void update_ring_id(osmium::object_id_type new_id) noexcept {
        if (new_id < m_ring_id) {
            m_ring_id = new_id;
        }
    }

    /// Returns the number of ways making up this ring.
    unsigned int nways() const noexcept {
        return m_nways;
    }

    /// Returns the number of points in this ring.
    unsigned int npoints() const noexcept {
        return m_way_node_list.size();
    }

    /// Returns true if the ring is closed.
    bool is_closed() const noexcept {
        return first_node_id() == last_node_id();
    }

    /// Was this ring fixed because of missing/wrong OSM data?
    bool is_fixed() const noexcept {
        return m_fixed;
    }

    /**
     * When there are two different nodes with the same location
     * a situation can arise where a CoastlineRing looks not closed
     * when looking at the node IDs but looks closed then looking
     * at the location. To "fix" this we change the node ID of the
     * last node in the ring to be the same as the first. This
     * method does this.
     */
    void fake_close() noexcept {
        assert(!m_way_node_list.empty());
        m_way_node_list.back().set_ref(first_node_id());
    }

    /**
     * Add pointers to the node locations to the given locmap. The
     * locmap can than later be used to directly put the locations
     * into the right place.
     */
    void setup_locations(locmap_type& locmap);

    /**
     * Check whether all node locations for the ways are there. This
     * can happen if the input data is missing a node needed for a
     * way. The function returns the number of missing locations.
     */
    unsigned int check_locations(bool output_missing);

    /// Add a new way to the front of this ring.
    void add_at_front(const osmium::Way& way);

    /// Add a new way to the end of this ring.
    void add_at_end(const osmium::Way& way);

    /**
     * Join the other ring to this one. The first node ID of the
     * other ring must be the same as the last node ID of this
     * ring.
     * The other ring can be destroyed afterwards.
     */
    void join(const CoastlineRing& other);

    /**
     * Join the other ring to this one, possibly over a gap.
     * The other ring can be destroyed afterwards.
     */
    void join_over_gap(const CoastlineRing& other);

    /**
     * Close ring by adding the first node to the end. If the
     * ring is already closed it is not changed.
     */
    void close_ring();

    /**
     * Close Antarctica ring by adding some nodes.
     */
    void close_antarctica_ring(int epsg);

    /**
     * Create OGRPolygon for this ring.
     *
     * Caller takes ownership of the created object.
     *
     * @param geom_factory Geometry factory that should be used to build the geometry.
     * @param reverse Reverse the ring when creating the geometry.
     */
    std::unique_ptr<OGRPolygon> ogr_polygon(osmium::geom::OGRFactory<>& geom_factory, bool reverse) const;

    /**
     * Create OGRLineString for this ring.
     *
     * Caller takes ownership of the created object.
     *
     * @param geom_factory Geometry factory that should be used to build the geometry.
     * @param reverse Reverse the ring when creating the geometry.
     */
    std::unique_ptr<OGRLineString> ogr_linestring(osmium::geom::OGRFactory<>& geom_factory, bool reverse) const;

    /**
     * Create OGRPoint for the first point in this ring.
     *
     * Caller takes ownership of created object.
     */
    std::unique_ptr<OGRPoint> ogr_first_point() const;

    /**
     * Create OGRPoint for the last point in this ring.
     loca*
     * Caller takes ownership of created object.
     */
    std::unique_ptr<OGRPoint> ogr_last_point() const;

    double distance_to_start_location(osmium::Location pos) const;

    void add_segments_to_vector(std::vector<osmium::UndirectedSegment>& segments) const;

    friend std::ostream& operator<<(std::ostream& out, const CoastlineRing& cp);

}; // class CoastlineRing

inline bool operator<(const CoastlineRing& lhs, const CoastlineRing& rhs) noexcept {
    return lhs.first_location() < rhs.first_location();
}

#endif // COASTLINE_RING_HPP
