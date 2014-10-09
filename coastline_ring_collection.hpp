#ifndef COASTLINE_RING_COLLECTION_HPP
#define COASTLINE_RING_COLLECTION_HPP

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

#include <list>
#include <vector>

#include <osmium/geom/ogr.hpp>
#include <osmium/osm/way.hpp>

#include "coastline_ring.hpp"

class OGRGeometry;
class OGRPolygon;
class OutputDatabase;
class CoastlinePolygons;

typedef std::list<std::shared_ptr<CoastlineRing>> coastline_rings_list_t;
typedef std::map<osmium::object_id_type, coastline_rings_list_t::iterator> idmap_t;

/**
 * A collection of CoastlineRing objects. Keeps a list of all start and end
 * nodes so it can efficiently join CoastlineRings.
 */
class CoastlineRingCollection {

    coastline_rings_list_t m_list;

    // Mapping from node IDs to CoastlineRings.
    idmap_t m_start_nodes;
    idmap_t m_end_nodes;

    unsigned int m_ways;
    unsigned int m_rings_from_single_way;
    unsigned int m_fixed_rings;

    void add_partial_ring(const osmium::Way& way);

    osmium::geom::OGRFactory<> m_factory;

public:

    typedef coastline_rings_list_t::const_iterator const_iterator;

    CoastlineRingCollection();

    /// Return the number of CoastlineRings in the collection.
    size_t size() const { return m_list.size(); }

    /**
     * Add way to collection. A new CoastlineRing will be created for the way
     * or it will be joined to an existing CoastlineRing.
     */
    void add_way(const osmium::Way& way) {
        m_ways++;
        if (way.is_closed()) {
            m_rings_from_single_way++;
            m_list.push_back(std::make_shared<CoastlineRing>(way));
        } else {
            add_partial_ring(way);
        }
    }

    unsigned int num_ways() const { return m_ways; }

    unsigned int num_rings_from_single_way() const { return m_rings_from_single_way; }

    unsigned int num_unconnected_nodes() const { return m_start_nodes.size() + m_end_nodes.size(); }

    unsigned int num_fixed_rings() const { return m_fixed_rings; }

    void setup_positions(posmap_type& posmap);

    unsigned int check_positions(bool output_missing);

    void add_polygons_to_vector(std::vector<OGRGeometry*>& vector);

    unsigned int output_rings(OutputDatabase& output);

    unsigned int check_for_intersections(OutputDatabase& output);

    bool close_antarctica_ring(int epsg);

    void close_rings(OutputDatabase& output, bool debug, double max_distance);

    unsigned int output_questionable(const CoastlinePolygons& polygons, OutputDatabase& output);

private:

    struct Connection {

        double distance;
        osmium::object_id_type start_id;
        osmium::object_id_type end_id;

        Connection(double d, osmium::object_id_type s, osmium::object_id_type e) : distance(d), start_id(s), end_id(e) { }

        /**
        * Returns true if start or end ID of this connection is the same as the
        * other connections start or end ID. Used as predicate in std::remove_if().
        */
        bool operator()(const Connection& other) {
            return start_id == other.start_id || end_id == other.end_id;
        }

        // Used in std::sort
        static bool sort_by_distance(const Connection& a, const Connection& b) { return a.distance > b.distance; }

    };

};

#endif // COASTLINE_RING_COLLECTION_HPP
