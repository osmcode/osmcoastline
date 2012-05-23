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
#include <getopt.h>
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

void print_help() {
    std::cout << "osmcoastline_filter [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help           - This help message\n"
              << "  -d, --debug          - Enable debugging output\n"
              << "  -o, --output=OSMFILE - Where to write output (default: none)\n"
              << "\n";
}

int main(int argc, char* argv[]) {
    bool debug = false;
    std::string output_filename;

    static struct option long_options[] = {
        {"debug",        no_argument, 0, 'd'},
        {"help",         no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "dho:", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
            case 'd':
                debug = true;
                break;
            case 'h':
                print_help();
                exit(0);
            case 'o':
                output_filename = optarg;
                break;
            default:
                exit(1);
        }
    }

    if (output_filename.empty()) {
        std::cerr << "Missing -o/--output=OSMFILE option\n";
        exit(1);
    }

    Osmium::init(debug);

    if (optind != argc - 1) {
        std::cerr << "Usage: osmcoastline_filter [OPTIONS] OSMFILE\n";
        exit(1);
    }

    try {
        Osmium::OSMFile outfile(output_filename);
        Osmium::Output::Base* output = outfile.create_output_file();

        idset ids;
        try {
            Osmium::OSMFile infile(argv[optind]);
            CoastlineFilterHandler1 handler1(output, ids);
            infile.read(handler1);
            CoastlineFilterHandler2 handler2(output, ids);
            infile.read(handler2);
        } catch (Osmium::OSMFile::IOError) {
            std::cerr << "Can not open input file '" << argv[optind] << "'\n";
            exit(1);
        }

        delete output;
    } catch (Osmium::OSMFile::IOError) {
        std::cerr << "Can not open output file '" << output_filename << "'\n";
        exit(1);
    }
}

