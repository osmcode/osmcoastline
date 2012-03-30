#ifndef COASTLINE_RING_COLLECTION_HPP
#define COASTLINE_RING_COLLECTION_HPP

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

#include <list>
#include <vector>
#include <boost/tr1/memory.hpp>
#include <boost/make_shared.hpp>

using std::tr1::shared_ptr;
using boost::make_shared;

#include <osmium/osm/way.hpp>

#include "coastline_ring.hpp"

class OGRPolygon;
class OutputDatabase;

typedef std::list< shared_ptr<CoastlineRing> > coastline_rings_list_t;
typedef std::map<osm_object_id_t, coastline_rings_list_t::iterator> idmap_t;

class CoastlineRingCollection {

    coastline_rings_list_t m_list;

    // Mapping from node IDs to CoastlineRings.
    idmap_t m_start_nodes;
    idmap_t m_end_nodes;

public:

    typedef coastline_rings_list_t::const_iterator const_iterator;

    CoastlineRingCollection();

    ~CoastlineRingCollection();

    size_t size() const {
        return m_list.size();
    }

    const_iterator begin() const {
        return m_list.begin();
    }

    const_iterator end() const {
        return m_list.end();
    }

    void add_complete_ring(const shared_ptr<Osmium::OSM::Way>& way) {
        m_list.push_back(make_shared<CoastlineRing>(way));
    }

    void add_partial_ring(const shared_ptr<Osmium::OSM::Way>& way);

    int number_of_unconnected_nodes() const {
        return m_start_nodes.size() + m_end_nodes.size();
    }

    void setup_positions(posmap_t& posmap);

    void add_polygons_to_vector(std::vector<OGRGeometry*>& vector);

    unsigned int output_rings(OutputDatabase& output);

};

#endif // COASTLINE_RING_COLLECTION_HPP
