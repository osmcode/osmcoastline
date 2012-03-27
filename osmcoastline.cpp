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

#include <cstdlib>
#include <fstream>
#include <list>
#include <time.h>
#include <getopt.h>

#include <osmium.hpp>
#include <osmium/geometry/point.hpp>

#include "osmcoastline.hpp"
#include "coastline_ring.hpp"
#include "output.hpp"
#include "output_layers.hpp"

/* ================================================== */

struct Options {

    std::string osmfile;
    std::string outdb;
    std::string raw_output;
    bool debug;
    bool create_index;

    Options(int argc, char* argv[]) :
        osmfile(),
        outdb(),
        raw_output(),
        debug(false),
        create_index(false)
    {
        static struct option long_options[] = {
            {"debug",        no_argument, 0, 'd'},
            {"help",         no_argument, 0, 'h'},
            {"create-index", no_argument, 0, 'I'},
            {"output",       required_argument, 0, 'o'},
            {"raw-output",   required_argument, 0, 'r'},
            {0, 0, 0, 0}
        };

        while (1) {
            int c = getopt_long(argc, argv, "dhIo:r:", long_options, 0);
            if (c == -1)
                break;

            switch (c) {
                case 'd':
                    debug = true;
                    std::cerr << "Enabled debug option\n";
                    break;
                case 'h':
                    print_help();
                    exit(return_code_ok);
                case 'I':
                    create_index = true;
                    break;
                case 'o':
                    outdb = optarg;
                    break;
                case 'r':
                    raw_output = optarg;
                    break;
                default:
                    exit(return_code_cmdline);
            }
        }

        if (optind != argc - 1) {
            std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE" << std::endl;
            exit(return_code_cmdline);
        }

        osmfile = argv[optind];
    }

    void print_help() {
        std::cout << "osmcoastline [OPTIONS] OSMFILE\n"
                  << "Options:\n"
                  << "  -h, --help                       - This help message\n"
                  << "  -d, --debug                      - Enable debugging output\n"
                  << "  -o, --output=DBFILE              - Spatialite database file for output\n"
                  << "  -r, --raw-output=OSMFILE         - Raw OSM output file\n"
                  << "  -I, --create-index               - Create spatial indexes in output database\n"
        ;
    }

};

/* ================================================== */

typedef std::list< shared_ptr<CoastlineRing> > coastline_rings_list_t;
typedef std::map<osm_object_id_t, coastline_rings_list_t::iterator> idmap_t;

/**
 * Osmium handler for the first pass over the input file in which
 * all ways tagged with 'natural=coastline' are read and CoastlineRings
 * are created.
 */
class CoastlineHandlerPass1 : public Osmium::Handler::Base {

    Osmium::Output::Base* m_raw_output;
    coastline_rings_list_t& m_coastline_rings;

    // Mapping from node IDs to CoastlineRings.
    idmap_t m_start_nodes;
    idmap_t m_end_nodes;

    // For statistics we keep track of how many coastline polygons were created
    // from a single ring.
    int m_count_polygons_from_single_way;

    unsigned int m_warnings;
    unsigned int m_errors;

public:

    CoastlineHandlerPass1(const Options& options, Osmium::Output::Base* raw_output, coastline_rings_list_t& clp) :
        m_raw_output(raw_output),
        m_coastline_rings(clp),
        m_start_nodes(),
        m_end_nodes(),
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

        // If the way is already closed we just create a CoastlineRing
        // for it and add it to the list.
        if (way->is_closed()) {
            m_count_polygons_from_single_way++;
            m_coastline_rings.push_back(make_shared<CoastlineRing>(way));
            return;
        }

        // If the way is not closed handling is a bit more complicated.
        // We'll check if there is an existing CoastlineRing where our
        // way would "fit" and add it to that ring.
        idmap_t::iterator mprev = m_end_nodes.find(way->get_first_node_id());
        idmap_t::iterator mnext = m_start_nodes.find(way->get_last_node_id());

        // There is no CoastlineRing yet where this way could fit,
        // create one.
        if (mprev == m_end_nodes.end() &&
            mnext == m_start_nodes.end()) {
            shared_ptr<CoastlineRing> cp = make_shared<CoastlineRing>(way);
            coastline_rings_list_t::iterator added = m_coastline_rings.insert(m_coastline_rings.end(), cp);
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
                m_coastline_rings.erase(next);
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

    void after_ways() const {
        std::cerr << "There are " << m_start_nodes.size() + m_end_nodes.size() << " nodes where the coastline is not closed.\n";
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
    coastline_rings_list_t& m_coastline_rings;
    posmap_t m_posmap;
    const Output& m_output;
    unsigned int m_warnings;
    unsigned int m_errors;

public:

    CoastlineHandlerPass2(const Options& options, Osmium::Output::Base* raw_output, coastline_rings_list_t& clp, const Output& output) :
        m_raw_output(raw_output),
        m_coastline_rings(clp),
        m_posmap(),
        m_output(output),
        m_warnings(0),
        m_errors(0)
    {
        for (coastline_rings_list_t::const_iterator it = m_coastline_rings.begin(); it != m_coastline_rings.end(); ++it) {
            (*it)->setup_positions(m_posmap);
        }
        if (m_raw_output) {
            m_raw_output->before_nodes();
        }
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        bool raw_out = false;

        if (m_output.layer_error_points()) {
            const char* natural = node->tags().get_tag_by_key("natural");
            if (natural && !strcmp(natural, "coastline")) {
                try {
                    Osmium::Geometry::Point point(*node);
                    m_output.layer_error_points()->add(point.create_ogr_geometry(), node->id(), "tagged_node");
                } catch (Osmium::Exception::IllegalGeometry) {
                    std::cerr << "Ignoring illegal geometry for node " << node->id() << ".\n";
                    m_errors++;
                }
            }
            raw_out = true;
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

/* ================================================== */

unsigned int output_rings(coastline_rings_list_t coastline_rings, const Output& output) {
    unsigned int warnings = 0;

    for (coastline_rings_list_t::const_iterator it = coastline_rings.begin(); it != coastline_rings.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed()) {
            if (cp.npoints() > 3) {
                output.layer_rings()->add(cp.ogr_polygon(), cp.min_way_id(), cp.nways(), cp.npoints(), output.layer_error_points());
            } else if (cp.npoints() == 1) {
                if (output.layer_error_points()) {
                    output.layer_error_points()->add(cp.ogr_first_point(), cp.first_node_id(), "single_point_in_ring");
                    warnings++;
                }
            } else { // cp.npoints() == 2 or 3
                if (output.layer_error_lines()) {
                    OGRLineString* l = cp.ogr_linestring();
                    output.layer_error_lines()->add(l, cp.min_way_id(), l->IsSimple());
                    warnings++;
                }
                if (output.layer_error_points()) {
                    output.layer_error_points()->add(cp.ogr_first_point(), cp.first_node_id(), "not_a_ring");
                    output.layer_error_points()->add(cp.ogr_last_point(), cp.last_node_id(), "not_a_ring");
                    warnings++;
                }
            }
        } else {
            if (output.layer_error_lines()) {
                OGRLineString* l = cp.ogr_linestring();
                output.layer_error_lines()->add(l, cp.min_way_id(), l->IsSimple());
                warnings++;
            }
            if (output.layer_error_points()) {
                output.layer_error_points()->add(cp.ogr_first_point(), cp.first_node_id(), "end_point");
                output.layer_error_points()->add(cp.ogr_last_point(), cp.last_node_id(), "end_point");
                warnings++;
            }
        }
    }

    return warnings;
}

/* ================================================== */

void output_polygons(coastline_rings_list_t coastline_rings, const Output& output) {
    std::vector<OGRGeometry*> all_polygons;
    all_polygons.reserve(coastline_rings.size());

    for (coastline_rings_list_t::const_iterator it = coastline_rings.begin(); it != coastline_rings.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed() && cp.npoints() > 3) {
            all_polygons.push_back(cp.ogr_polygon());
        }
    }

    int is_valid;
    const char* options[] = { "METHOD=ONLY_CCW", NULL };
    OGRGeometry* mega_multipolygon = OGRGeometryFactory::organizePolygons(&all_polygons[0], all_polygons.size(), &is_valid, options);

    assert(mega_multipolygon->getGeometryType() == wkbMultiPolygon);

    for (int i=0; i < static_cast<OGRMultiPolygon*>(mega_multipolygon)->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(static_cast<OGRMultiPolygon*>(mega_multipolygon)->getGeometryRef(i));
        output.layer_polygons()->add(p, p->getExteriorRing()->isClockwise());
    }
}

/* ================================================== */

void print_memory_usage() {
    char filename[100];
    sprintf(filename, "/proc/%d/status", getpid());
    std::ifstream status_file(filename);
    std::string line;

    if (status_file.is_open()) {
        while (! status_file.eof() ) {
            std::getline(status_file, line);
            if (line.substr(0, 6) == "VmPeak" || line.substr(0, 6) == "VmSize") {
                std::cerr << line << std::endl;
            }
        }
        status_file.close();
    }
}

/* ================================================== */

int main(int argc, char *argv[]) {
    unsigned int warnings = 0;
    unsigned int errors = 0;

    Options options(argc, argv);

    Osmium::init(options.debug);

    if (options.debug) {
        std::cerr << "Reading from file '" << options.osmfile << "'.\n";
    }
    Osmium::OSMFile infile(options.osmfile);

    Osmium::OSMFile* outfile = NULL;
    Osmium::Output::Base* raw_output = NULL;
    if (options.raw_output.empty()) {
        if (options.debug) {
            std::cerr << "Not writing raw output.\n";
        }
    } else {
        if (options.debug) {
            std::cerr << "Writing raw output to file '" << options.raw_output << "'.\n";
        }
        outfile = new Osmium::OSMFile(options.raw_output);
        raw_output = outfile->create_output_file();
    }

    coastline_rings_list_t coastline_rings;

    Output output(options.outdb, options.create_index);
    output.create_layer_error_points();
    output.create_layer_error_lines();
    output.create_layer_rings();
    output.create_layer_polygons();

    std::cerr << "-------------------------------------------------------------------------------\n";
    std::cerr << "Reading ways (1st pass through input file)...\n";
    time_t t = time(NULL);
    CoastlineHandlerPass1 handler_pass1(options, raw_output, coastline_rings);
    infile.read(handler_pass1);
    std::cerr << "Done (about " << (time(NULL)-t)/60 << " minutes).\n";
    print_memory_usage();

    warnings += handler_pass1.warnings();
    errors += handler_pass1.errors();

    std::cerr << "-------------------------------------------------------------------------------\n";
    std::cerr << "Reading nodes (2nd pass through input file)...\n";
    t = time(NULL);
    CoastlineHandlerPass2 handler_pass2(options, raw_output, coastline_rings, output);
    infile.read(handler_pass2);
    std::cerr << "Done (about " << (time(NULL)-t)/60 << " minutes).\n";
    print_memory_usage();

    warnings += handler_pass2.warnings();
    errors += handler_pass2.errors();

    std::cerr << "-------------------------------------------------------------------------------\n";
    std::cerr << "Create and output polygons...\n";
    t = time(NULL);
    if (output.layer_rings()) {
        warnings += output_rings(coastline_rings, output);
    }
    if (output.layer_polygons()) {
        output_polygons(coastline_rings, output);
    }
    std::cerr << "Done (about " << (time(NULL)-t)/60 << " minutes).\n";
    print_memory_usage();

    std::cerr << "-------------------------------------------------------------------------------\n";

    delete raw_output;
    delete outfile;

    if (warnings) {
        std::cerr << "There were " << warnings << " warnings.\n";
    }
    if (errors) {
        std::cerr << "There were " << errors << " errors.\n";
    }
    if (errors) {
        exit(return_code_error);
    } else if (warnings) {
        exit(return_code_warning);
    } else {
        exit(return_code_ok);
    }
}

