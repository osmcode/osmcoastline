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
#include "output_database.hpp"

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

unsigned int CoastlineRingCollection::output_rings(OutputDatabase& output) {
    unsigned int warnings = 0;

    for (coastline_rings_list_t::const_iterator it = m_list.begin(); it != m_list.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed()) {
            if (cp.npoints() > 3) {
                output.add_ring(cp.ogr_polygon(), cp.ring_id(), cp.nways(), cp.npoints());
            } else if (cp.npoints() == 1) {
                output.add_error(cp.ogr_first_point(), "single_point_in_ring", cp.first_node_id());
                warnings++;
            } else { // cp.npoints() == 2 or 3
                output.add_error(cp.ogr_linestring(), "not_a_ring", cp.ring_id());
                output.add_error(cp.ogr_first_point(), "not_a_ring", cp.first_node_id());
                output.add_error(cp.ogr_last_point(), "not_a_ring", cp.last_node_id());
                warnings++;
            }
        } else {
            output.add_error(cp.ogr_linestring(), "not_closed", cp.ring_id());
            output.add_error(cp.ogr_first_point(), "end_point", cp.first_node_id());
            output.add_error(cp.ogr_last_point(), "end_point", cp.last_node_id());
            warnings++;
        }
    }

    return warnings;
}

class Connection {

public:

    double m_distance;
    idmap_t::iterator m_eit;
    idmap_t::iterator m_sit;

    Connection(double distance, idmap_t::iterator eit, idmap_t::iterator sit) :
        m_distance(distance),
        m_eit(eit),
        m_sit(sit)
    {
    }

    void make_connection() const {
        CoastlineRing* e = m_eit->second->get();
        CoastlineRing* s = m_sit->second->get();

        if (e == s) {
            // connect to itself by closing ring
            e->close_ring();
        } else {
            // connect to other ring
            e->join_over_gap(*s);
        }
    }

    double distance() const {
        return m_distance;
    }

    bool operator()(const Connection& other) {
        return m_eit == other.m_eit || m_sit == other.m_sit;
    }

    osm_object_id_t start_id() const {
        return m_sit->first;
    }

    osm_object_id_t end_id() const {
        return m_eit->first;
    }

};

bool sort_by_distance(const Connection& a, const Connection& b) {
    return a.distance() > b.distance();
}

const double max_distance = 1;

typedef std::vector<Connection> connections_t;

void CoastlineRingCollection::close_rings() {
    std::cerr << "close_rings():\n";
    connections_t connections;

    // create vector with all possible combinations of connections between rings
    for (idmap_t::iterator eit = m_end_nodes.begin(); eit != m_end_nodes.end(); ++eit) {
        for (idmap_t::iterator sit = m_start_nodes.begin(); sit != m_start_nodes.end(); ++sit) {
            double distance = (*sit->second)->distance_to_start_position((*eit->second)->last_position());
            if (distance < max_distance) {
                connections.push_back(Connection(distance, eit, sit));
            }
        }
    }

    // sort vector by distance
    std::sort(connections.begin(), connections.end(), sort_by_distance);

    // go through vector and close rings using the connections in turn
    while (!connections.empty()) {
        Connection c = connections.back();
        connections.pop_back();

        // invalidate all other connections using one of the same end points
        connections.erase(remove_if(connections.begin(), connections.end(), c), connections.end());

        std::cerr << "  close between " << c.end_id() << " and " << c.start_id() << "\n";
        c.make_connection();        
    }
}

