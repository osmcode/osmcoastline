#ifndef COASTLINE_RING_HPP
#define COASTLINE_RING_HPP

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

#include <map>

#include <osmium/osm/way.hpp>

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

    osm_object_id_t m_first_node_id;
    osm_object_id_t m_last_node_id;

    /// Minimum ID of all the ways making up the ring.
    osm_object_id_t m_min_way_id;

    /// The number of ways making up this ring.
    unsigned int m_nways;

public:

    CoastlineRing(const shared_ptr<Osmium::OSM::Way>& way) :
        m_way_node_list(way->is_closed() ? way->node_count() : 1000),
        m_first_node_id(way->get_first_node_id()),
        m_last_node_id(way->get_last_node_id()),
        m_min_way_id(way->id()),
        m_nways(1)
    {
        m_way_node_list.insert(m_way_node_list.begin(), way->nodes().begin(), way->nodes().end());
    }

    /**
     * Join the other ring to this one. The other ring can be destroyed
     * afterwards.
     */
    void join(const CoastlineRing& other);

    void join_over_gap(const CoastlineRing& other);

    /// Add a new way to the end of this ring.
    void add_at_end(const shared_ptr<Osmium::OSM::Way>& way);

    /// Add a new way to the front of this ring.
    void add_at_front(const shared_ptr<Osmium::OSM::Way>& way);

    void close_ring();

    osm_object_id_t first_node_id() const { return m_first_node_id; }
    osm_object_id_t last_node_id()  const { return m_last_node_id; }
    osm_object_id_t min_way_id()    const { return m_min_way_id; }

    /// Returns the number of ways making up this ring.
    unsigned int nways() const { return m_nways; }

    /// Returns the number of points in this ring.
    unsigned int npoints() const { return m_way_node_list.size(); }

    /// Returns true if the ring is closed.
    bool is_closed() const { return first_node_id() == last_node_id(); }

    /**
     * Add pointers to the node positions to the given posmap. The
     * posmap can than later be used to directly put the positions
     * into the right place.
     */
    void setup_positions(posmap_t& posmap);

    /**
     * Create OGRPolygon for this ring. The ring is reversed in the
     * process.
     *
     * Caller takes ownership of created object.
     */
    OGRPolygon* ogr_polygon() const;

    /**
     * Create OGRLineString for this ring. The ring is reversed in the
     * process.
     *
     * Caller takes ownership of created object.
     */
    OGRLineString* ogr_linestring() const;

    /**
     * Create OGRPoint for this first point in this ring. (Because
     * of the ring reversal this is actually the last point of the
     * last way used to create the CoastlineRing.)
     *
     * Caller takes ownership of created object.
     */
    OGRPoint* ogr_first_point() const;

    /**
     * Create OGRPoint for this last point in this ring. (Because
     * of the ring reversal this is actually the last point of the
     * last way used to create the CoastlineRing.)
     *
     * Caller takes ownership of created object.
     */
    OGRPoint* ogr_last_point() const;

    Osmium::OSM::Position start_position() const;
    Osmium::OSM::Position end_position() const;
    double distance_to_start_position(Osmium::OSM::Position pos) const;

    friend std::ostream& operator<<(std::ostream& out, const CoastlineRing& cp);
};

#endif // COASTLINE_RING_HPP
