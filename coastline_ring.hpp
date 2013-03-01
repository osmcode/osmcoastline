#ifndef COASTLINE_RING_HPP
#define COASTLINE_RING_HPP

/*

  Copyright 2013 Jochen Topf <jochen@topf.org>.

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

#include <map>

#include <osmium/osm/way.hpp>
#include <osmium/osm/undirected_segment.hpp>

class OGRPoint;
class OGRLineString;
class OGRPolygon;

typedef std::multimap<osm_object_id_t, Osmium::OSM::Position*> posmap_t;

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

    Osmium::OSM::WayNodeList m_way_node_list;

    /**
     * Smallest ID of all the ways making up the ring. Can be used as somewhat
     * stable unique ID for the ring.
     */
    osm_object_id_t m_ring_id;

    /**
     * The number of ways making up this ring. This is not actually needed for
     * anything, but kept to create statistics.
     */
    unsigned int m_nways;

    /// Ring was fixed because of missing/wrong OSM data.
    bool m_fixed;

    /// Is this an outer ring?
    bool m_outer;

public:

    /**
     * Create CoastlineRing from a way.
     */
    CoastlineRing(const shared_ptr<Osmium::OSM::Way>& way) :
        m_way_node_list(way->is_closed() ? way->nodes().size() : 1000),
        m_ring_id(way->id()),
        m_nways(1),
        m_fixed(false),
        m_outer(false)
    {
        m_way_node_list.insert(m_way_node_list.begin(), way->nodes().begin(), way->nodes().end());
    }

    bool is_outer() const {
        return m_outer;
    }

    void set_outer() {
        m_outer = true;
    }

    /// ID of first node in the ring.
    osm_object_id_t first_node_id() const { return m_way_node_list.front().ref(); }

    /// ID of last node in the ring.
    osm_object_id_t last_node_id() const { return m_way_node_list.back().ref(); }

    /// Position of the first node in the ring.
    Osmium::OSM::Position first_position() const { return m_way_node_list.front().position(); }

    /// Position of the last node in the ring.
    Osmium::OSM::Position last_position() const { return m_way_node_list.back().position(); }

    /// Return ID of this ring (defined as smallest ID of the ways making up the ring).
    osm_object_id_t ring_id() const { return m_ring_id; }

    /**
     * Set ring ID. The ring will only get the new ID if it is smaller than the
     * old one.
     */
    void update_ring_id(osm_object_id_t new_id) {
        if (new_id < m_ring_id) {
            m_ring_id = new_id;
        }
    }

    /// Returns the number of ways making up this ring.
    unsigned int nways() const { return m_nways; }

    /// Returns the number of points in this ring.
    unsigned int npoints() const { return m_way_node_list.size(); }

    /// Returns true if the ring is closed.
    bool is_closed() const { return first_node_id() == last_node_id(); }

    /// Was this ring fixed because of missing/wrong OSM data?
    bool is_fixed() const { return m_fixed; }

    /**
     * When there are two different nodes with the same position
     * a situation can arise where a CoastlineRing looks not closed
     * when looking at the node IDs but looks closed then looking
     * at the positions. To "fix" this we change the node ID of the
     * last node in the ring to be the same as the first. This
     * method does this.
     */
    void fake_close() { m_way_node_list.back().ref(first_node_id()); }

    /**
     * Add pointers to the node positions to the given posmap. The
     * posmap can than later be used to directly put the positions
     * into the right place.
     */
    void setup_positions(posmap_t& posmap);

    /// Add a new way to the front of this ring.
    void add_at_front(const shared_ptr<Osmium::OSM::Way>& way);

    /// Add a new way to the end of this ring.
    void add_at_end(const shared_ptr<Osmium::OSM::Way>& way);

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
     * @param reverse Reverse the ring when creating the geometry.
     */
    OGRPolygon* ogr_polygon(bool reverse) const;

    /**
     * Create OGRLineString for this ring.
     *
     * Caller takes ownership of the created object.
     *
     * @param reverse Reverse the ring when creating the geometry.
     */
    OGRLineString* ogr_linestring(bool reverse) const;

    /**
     * Create OGRPoint for the first point in this ring.
     *
     * Caller takes ownership of created object.
     */
    OGRPoint* ogr_first_point() const;

    /**
     * Create OGRPoint for the last point in this ring.
     *
     * Caller takes ownership of created object.
     */
    OGRPoint* ogr_last_point() const;

    double distance_to_start_position(Osmium::OSM::Position pos) const;

    void add_segments_to_vector(std::vector<Osmium::OSM::UndirectedSegment>& segments) const;

    friend std::ostream& operator<<(std::ostream& out, const CoastlineRing& cp);

};

inline bool operator<(const CoastlineRing& lhs, const CoastlineRing& rhs) {
    return lhs.first_position() < rhs.first_position();
}

#endif // COASTLINE_RING_HPP
