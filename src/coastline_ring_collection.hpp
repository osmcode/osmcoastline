#ifndef COASTLINE_RING_COLLECTION_HPP
#define COASTLINE_RING_COLLECTION_HPP

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
#include <osmium/osm/way.hpp>
#include <osmium/osm/types.hpp>

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <vector>

class OGRGeometry;
class OutputDatabase;
class CoastlinePolygons;

using coastline_rings_list_type = std::list<std::shared_ptr<CoastlineRing>>;
using idmap_type = std::map<osmium::object_id_type, coastline_rings_list_type::iterator>;

/**
 * A collection of CoastlineRing objects. Keeps a list of all start and end
 * nodes so it can efficiently join CoastlineRings.
 */
class CoastlineRingCollection {

    coastline_rings_list_type m_list;

    // Mapping from node IDs to CoastlineRings.
    idmap_type m_start_nodes;
    idmap_type m_end_nodes;

    unsigned int m_ways = 0;
    unsigned int m_rings_from_single_way = 0;
    unsigned int m_fixed_rings = 0;

    void add_partial_ring(const osmium::Way& way);

    osmium::geom::OGRFactory<> m_factory;

public:

    using const_iterator = coastline_rings_list_type::const_iterator;

    CoastlineRingCollection() = default;

    /// Return the number of CoastlineRings in the collection.
    std::size_t size() const noexcept {
        return m_list.size();
    }

    /**
     * Add way to collection. A new CoastlineRing will be created for the way
     * or it will be joined to an existing CoastlineRing.
     */
    void add_way(const osmium::Way& way) {
        assert(!way.nodes().empty());
        m_ways++;
        if (way.is_closed()) {
            m_rings_from_single_way++;
            m_list.push_back(std::make_shared<CoastlineRing>(way));
        } else {
            add_partial_ring(way);
        }
    }

    unsigned int num_ways() const noexcept {
        return m_ways;
    }

    unsigned int num_rings_from_single_way() const noexcept {
        return m_rings_from_single_way;
    }

    unsigned int num_unconnected_nodes() const noexcept {
        return m_start_nodes.size() + m_end_nodes.size();
    }

    unsigned int num_fixed_rings() const noexcept {
        return m_fixed_rings;
    }

    void setup_locations(locmap_type& locmap);

    unsigned int check_locations(bool output_missing);

    std::vector<OGRGeometry*> add_polygons_to_vector();

    unsigned int output_rings(OutputDatabase& output);

    unsigned int check_for_intersections(OutputDatabase& output, int segments_fd);

    bool close_antarctica_ring(int epsg);

    void close_rings(OutputDatabase& output, bool debug, double max_distance);

    unsigned int output_questionable(const CoastlinePolygons& polygons, OutputDatabase& output);

private:

    struct Connection {

        double distance;
        osmium::object_id_type start_id;
        osmium::object_id_type end_id;

        Connection(double d, osmium::object_id_type s, osmium::object_id_type e) :
            distance(d),
            start_id(s),
            end_id(e) {
        }

        /**
        * Returns true if start or end ID of this connection is the same as the
        * other connections start or end ID. Used as predicate in std::remove_if().
        */
        bool operator()(const Connection& other) const noexcept {
            return start_id == other.start_id || end_id == other.end_id;
        }

        // Used in std::sort
        static bool sort_by_distance(const Connection& a, const Connection& b) noexcept {
            return a.distance > b.distance;
        }

    }; // struct Connection

}; // class CoastlineRingCollection

#endif // COASTLINE_RING_COLLECTION_HPP
