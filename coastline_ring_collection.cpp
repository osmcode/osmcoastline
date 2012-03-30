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

#include "coastline_ring_collection.hpp"

CoastlineRingCollection::CoastlineRingCollection() :
    m_list(),
    m_start_nodes(),
    m_end_nodes() {
}

CoastlineRingCollection::~CoastlineRingCollection() {
}

// If the way is not closed handling is a bit more complicated.
// We'll check if there is an existing CoastlineRing where our
// way would "fit" and add it to that ring.
void CoastlineRingCollection::add_partial_ring(const shared_ptr<Osmium::OSM::Way>& way) {
    idmap_t::iterator mprev = m_end_nodes.find(way->get_first_node_id());
    idmap_t::iterator mnext = m_start_nodes.find(way->get_last_node_id());

    // There is no CoastlineRing yet where this way could fit,
    // create one.
    if (mprev == m_end_nodes.end() &&
        mnext == m_start_nodes.end()) {
        shared_ptr<CoastlineRing> cp = make_shared<CoastlineRing>(way);
        coastline_rings_list_t::iterator added = m_list.insert(m_list.end(), cp);
        m_start_nodes[way->get_first_node_id()] = added;
        m_end_nodes[way->get_last_node_id()] = added;
        return;
    }

    // We found a CoastlineRing where we can add the way at the end.
    if (mprev != m_end_nodes.end()) {
        coastline_rings_list_t::iterator prev = mprev->second;
        (*prev)->add_at_end(way);
        m_end_nodes.erase(mprev);

        if ((*prev)->is_closed()) {
            idmap_t::iterator x = m_start_nodes.find((*prev)->first_node_id());
            m_start_nodes.erase(x);
            return;
        }

        // We also found a CoastlineRing where we could have added the
        // way at the front. This means that the way together with the
        // ring at front and the ring at back are now a complete ring.
        if (mnext != m_start_nodes.end()) {
            coastline_rings_list_t::iterator next = mnext->second;
            (*prev)->join(**next);
            m_start_nodes.erase(mnext);
            if ((*prev)->is_closed()) {
                idmap_t::iterator x = m_start_nodes.find((*prev)->first_node_id());
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
        coastline_rings_list_t::iterator next = mnext->second;
        (*next)->add_at_front(way);
        m_start_nodes.erase(mnext);
        if ((*next)->is_closed()) {
            idmap_t::iterator x = m_end_nodes.find((*next)->last_node_id());
            m_end_nodes.erase(x);
            return;
        }
        m_start_nodes[(*next)->first_node_id()] = next;
    }
}

void CoastlineRingCollection::setup_positions(posmap_t& posmap) {
    for (coastline_rings_list_t::const_iterator it = m_list.begin(); it != m_list.end(); ++it) {
        (*it)->setup_positions(posmap);
    }
}

void CoastlineRingCollection::add_polygons_to_vector(std::vector<OGRGeometry*>& vector) {
    vector.reserve(m_list.size());

    for (coastline_rings_list_t::const_iterator it = m_list.begin(); it != m_list.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed() && cp.npoints() > 3) { // XXX what's with rings that don't match here?
            vector.push_back(cp.ogr_polygon());
        }
    }
}

