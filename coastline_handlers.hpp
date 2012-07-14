#ifndef COASTLINE_HANDLERS_HPP
#define COASTLINE_HANDLERS_HPP

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

#include <osmium/geometry/ogr.hpp>

/**
 * Osmium handler for the first pass over the input file in which
 * all ways tagged with 'natural=coastline' are read and CoastlineRings
 * are created.
 */
class CoastlineHandlerPass1 : public Osmium::Handler::Base {

    CoastlineRingCollection& m_coastline_rings;

public:

    CoastlineHandlerPass1(CoastlineRingCollection& coastline_rings) :
        m_coastline_rings(coastline_rings)
    {
    }

    void way(const shared_ptr<Osmium::OSM::Way>& way) {
        // We are only interested in ways tagged with natural=coastline.
        const char* natural = way->tags().get_tag_by_key("natural");
        if (natural && !strcmp(natural, "coastline")) {
            m_coastline_rings.add_way(way);
        }
    }

    void after_ways() const {
        // We only need to read ways in this pass.
        throw Osmium::Handler::StopReading();
    }

};

/**
 * Osmium handler for the second pass over the input file in which
 * node coordinates are added to the CoastlineRings.
 */
class CoastlineHandlerPass2 : public Osmium::Handler::Base {

    CoastlineRingCollection& m_coastline_rings;

    /**
     * Multimap for a mapping from node ID to all places where the
     * position of this node should be written to. Those places
     * are in the CoastlineRings created from the ways. This map
     * is set up first thing when the handler is instantiated and
     * thereafter used for each node coming in.
     */
    posmap_t m_posmap;
    OutputDatabase& m_output;

public:

    CoastlineHandlerPass2(CoastlineRingCollection& coastline_rings, OutputDatabase& output) :
        m_coastline_rings(coastline_rings),
        m_posmap(),
        m_output(output)
    {
        m_coastline_rings.setup_positions(m_posmap);
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        const char* natural = node->tags().get_tag_by_key("natural");
        if (natural && !strcmp(natural, "coastline")) {
            try {
                Osmium::Geometry::Point point(*node);
                m_output.add_error_point(Osmium::Geometry::create_ogr_geometry(point), "tagged_node", node->id());
            } catch (Osmium::Exception::IllegalGeometry) {
                std::cerr << "Ignoring illegal geometry for node " << node->id() << ".\n";
            }
        }

        std::pair<posmap_t::iterator, posmap_t::iterator> ret = m_posmap.equal_range(node->id());
        for (posmap_t::iterator it=ret.first; it != ret.second; ++it) {
            *(it->second) = node->position();
        }
    }

    void after_nodes() const {
        // We only need to read nodes in this pass.
        throw Osmium::Handler::StopReading();
    }

};

#endif // COASTLINE_HANDLERS_HPP
