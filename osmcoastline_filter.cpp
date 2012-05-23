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

#include <iostream>
#include <set>
#include <osmium.hpp>

#include "osmcoastline.hpp"

typedef std::set<osm_object_id_t> idset;

/**
 * Handler for first pass. Find all ways tagged with natural=coastline
 * and write them out. Also remember all nodes needed for those ways.
 */
class CoastlineFilterHandler1 : public Osmium::Handler::Base {

    Osmium::Output::Base* m_output;
    idset& m_ids;

public:

    CoastlineFilterHandler1(Osmium::Output::Base* output, idset& ids) :
        m_output(output),
        m_ids(ids) {
    }

    void init(Osmium::OSM::Meta& meta) {
        m_output->init(meta);
    }

    void before_ways() const {
        m_output->before_ways();
    }

    void way(const shared_ptr<Osmium::OSM::Way const>& way) const {
        const char* natural = way->tags().get_tag_by_key("natural");
        if (natural && !strcmp(natural, "coastline")) {
            m_output->way(way);
            for (Osmium::OSM::WayNodeList::const_iterator it = way->nodes().begin(); it != way->nodes().end(); ++it) {
                m_ids.insert(it->ref());
            }
        }
    }

    void after_ways() const {
        m_output->after_ways();

        // We only need to read ways on the first pass
        throw Osmium::Input::StopReading();
    }

};

/**
 * Handler for the second pass. Write out all nodes that are tagged
 * natural=coastline or that we need for the ways we found in the first
 * pass.
 */
class CoastlineFilterHandler2 : public Osmium::Handler::Base {

    Osmium::Output::Base* m_output;
    idset& m_ids;

public:

    CoastlineFilterHandler2(Osmium::Output::Base* output, idset& ids) :
        m_output(output),
        m_ids(ids) {
    }

    void before_nodes() const {
        m_output->before_nodes();
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) const {
        const char* natural = node->tags().get_tag_by_key("natural");
        if ((m_ids.find(node->id()) != m_ids.end()) || (natural && !strcmp(natural, "coastline"))) {
            m_output->node(node);
        }
    }

    void after_nodes() const {
        m_output->after_nodes();
        m_output->final();

        // We only need to read nodes on the second pass
        throw Osmium::Input::StopReading();
    }

};

int main(int argc, char* argv[]) {
    Osmium::init(false);

    if (argc != 3) {
        std::cerr << "Usage: osmcoastline_filter INFILE OUTFILE\n";
        exit(return_code_cmdline);
    }

    Osmium::OSMFile outfile(argv[2]);
    Osmium::Output::Base* output = outfile.create_output_file();

    idset ids;
    Osmium::OSMFile infile(argv[1]);
    CoastlineFilterHandler1 handler1(output, ids);
    infile.read(handler1);
    CoastlineFilterHandler2 handler2(output, ids);
    infile.read(handler2);

    delete output;
}

