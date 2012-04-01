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

/**
 * Osmium handler for the first pass over the input file in which
 * all ways tagged with 'natural=coastline' are read and CoastlineRings
 * are created.
 */
class CoastlineHandlerPass1 : public Osmium::Handler::Base {

    Osmium::Output::Base* m_raw_output;
    CoastlineRingCollection& m_coastline_rings;

    // For statistics we keep track of how many coastline polygons were created
    // from a single ring.
    int m_count_polygons_from_single_way;

    unsigned int m_warnings;
    unsigned int m_errors;

public:

    CoastlineHandlerPass1(Osmium::Output::Base* raw_output, CoastlineRingCollection& coastline_rings) :
        m_raw_output(raw_output),
        m_coastline_rings(coastline_rings),
        m_count_polygons_from_single_way(0),
        m_warnings(0),
        m_errors(0)
    {
    }

    void init(Osmium::OSM::Meta& meta) {
        if (m_raw_output) {
            m_raw_output->init(meta);
            m_raw_output->before_ways();
        }
    }

    void way(const shared_ptr<Osmium::OSM::Way>& way) {
        // We are only interested in ways tagged with natural=coastline.
        const char* natural = way->tags().get_tag_by_key("natural");
        if (!natural || strcmp(natural, "coastline")) {
            return;
        }

        if (m_raw_output) {
            m_raw_output->way(way);
        }

        if (way->is_closed()) {
            m_count_polygons_from_single_way++;
        }

        m_coastline_rings.add_way(way);
    }

    void after_ways() const {
        std::cerr << "There are " << m_coastline_rings.number_of_unconnected_nodes() << " nodes where the coastline is not closed.\n";
        std::cerr << "There are " << m_coastline_rings.size() << " coastline rings ("
            << m_count_polygons_from_single_way << " from a single way and "
            << m_coastline_rings.size() - m_count_polygons_from_single_way << " from multiple ways).\n";

        if (m_raw_output) {
            m_raw_output->after_ways();
        }

        // We only need to read ways in this pass.
        throw Osmium::Input::StopReading();
    }

    unsigned int errors() const {
        return m_errors;
    }

    unsigned int warnings() const {
        return m_warnings;
    }
};

/* ================================================== */

/**
 * Osmium handler for the second pass over the input file in which
 * node coordinates are added to the CoastlineRings.
 */
class CoastlineHandlerPass2 : public Osmium::Handler::Base {

    Osmium::Output::Base* m_raw_output;
    CoastlineRingCollection& m_coastline_rings;
    posmap_t m_posmap;
    OutputDatabase* m_output;
    unsigned int m_warnings;
    unsigned int m_errors;

public:

    CoastlineHandlerPass2(Osmium::Output::Base* raw_output, CoastlineRingCollection& coastline_rings, OutputDatabase* output) :
        m_raw_output(raw_output),
        m_coastline_rings(coastline_rings),
        m_posmap(),
        m_output(output),
        m_warnings(0),
        m_errors(0)
    {
        m_coastline_rings.setup_positions(m_posmap);
        if (m_raw_output) {
            m_raw_output->before_nodes();
        }
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        bool raw_out = false;

        if (m_output) {
            const char* natural = node->tags().get_tag_by_key("natural");
            if (natural && !strcmp(natural, "coastline")) {
                raw_out = true;
                try {
                    Osmium::Geometry::Point point(*node);
                    m_output->add_error(point.create_ogr_geometry(), "tagged_node", node->id());
                } catch (Osmium::Exception::IllegalGeometry) {
                    std::cerr << "Ignoring illegal geometry for node " << node->id() << ".\n";
                    m_errors++;
                }
            }
        }

        std::pair<posmap_t::iterator, posmap_t::iterator> ret = m_posmap.equal_range(node->id());
        for (posmap_t::iterator it=ret.first; it != ret.second; ++it) {
            *(it->second) = node->position();
            raw_out = true;
        }

        if (m_raw_output && raw_out) {
            m_raw_output->node(node);
        }
    }

    void after_nodes() const {
        if (m_raw_output) {
            m_raw_output->after_nodes();
            m_raw_output->final();
        }

        // We only need to read nodes in this pass.
        throw Osmium::Input::StopReading();
    }

    unsigned int errors() const {
        return m_errors;
    }

    unsigned int warnings() const {
        return m_warnings;
    }
};

#endif // COASTLINE_HANDLERS_HPP
