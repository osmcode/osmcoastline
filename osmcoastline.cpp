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
#include "output_database.hpp"
#include "output_layers.hpp"

/* ================================================== */

/**
 * This class encapsulates the command line parsing.
 */
struct Options {

    /// Input OSM file name.
    std::string osmfile;

    /// Output Spatialite database file name.
    std::string outdb;

    /// Output OSM "raw data" file name.
    std::string raw_output;

    /// EPSG code of output SRS.
    int epsg;

    /// Should the rings output table be populated?
    bool output_rings;

    /// Should the polygons output table be populated?
    bool output_polygons;

    /// Should large polygons be split?
    bool split_large_polygons;

    /// Maximum number of points in polygons
    int max_points_in_polygons;

    /// Show debug output?
    bool debug;

    /// Add spatial index to Spatialite database tables?
    bool create_index;

    Options(int argc, char* argv[]) :
        osmfile(),
        outdb(),
        raw_output(),
        epsg(4326),
        output_rings(false),
        output_polygons(true),
        split_large_polygons(false),
        max_points_in_polygons(1000),
        debug(false),
        create_index(false)
    {
        static struct option long_options[] = {
            {"debug",           no_argument, 0, 'd'},
            {"help",            no_argument, 0, 'h'},
            {"create-index",    no_argument, 0, 'I'},
            {"output",          required_argument, 0, 'o'},
            {"output-rings",    no_argument, 0, 'R'},
            {"output-polygons", required_argument, 0, 'P'},
            {"raw-output",      required_argument, 0, 'r'},
            {"split",           optional_argument, 0, 'S'},
            {"srs",             required_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        while (1) {
            int c = getopt_long(argc, argv, "dhIo:r:Rs:S:P:", long_options, 0);
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
                case 'P':
                    output_polygons = false;
                    break;
                case 'r':
                    raw_output = optarg;
                    break;
                case 'R':
                    output_rings = true;
                    break;
                case 's':
                    epsg = get_epsg(optarg);
                    break;
                case 'S':
                    split_large_polygons = true;
                    if (optarg) {
                        max_points_in_polygons = atoi(optarg);
                    }
                    break;
                default:
                    exit(return_code_cmdline);
            }
        }

        if (optind != argc - 1) {
            std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE" << std::endl;
            exit(return_code_cmdline);
        }

        if (outdb.empty() && raw_output.empty()) {
            std::cerr << "You have to give one or both of the --output or --raw-output options.\n";
            exit(return_code_cmdline);
        }

        osmfile = argv[optind];
    }

    int get_epsg(const char* text) {
        if (!strcasecmp(text, "WGS84") || !strcmp(text, "4326")) {
            return 4326;
        }
        if (!strcmp(text, "3857")) {
            return 3857;
        }
        if (!strcmp(text, "3785") || !strcmp(text, "900913")) {
            std::cerr << "Please use code 3857 for the 'Google Mercator' projection!\n";
            exit(return_code_cmdline);
        }
        std::cerr << "Unknown SRS '" << text << "'. Currently only 4326 (WGS84) and 3857 ('Google Mercator') are supported.\n";
        exit(return_code_cmdline);
    }

    void print_help() {
        std::cout << "osmcoastline [OPTIONS] OSMFILE\n"
                  << "Options:\n"
                  << "  -d, --debug              - Enable debugging output\n"
                  << "  -h, --help               - This help message\n"
                  << "  -I, --create-index       - Create spatial indexes in output database\n"
                  << "  -o, --output=DBFILE      - Spatialite database file for output\n"
                  << "  -r, --raw-output=OSMFILE - Write raw OSM output file\n"
                  << "  -s, --srs=EPSGCODE       - Set output SRS (4326 for WGS84 (default) or 3857)\n"
                  << "  -S, --split[=N]          - Split polygons with more than N (default 1000) points\n"
                  << "\n"
                  << "At least -o and/or -r is needed.\n"
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
    OutputDatabase* m_output;
    unsigned int m_warnings;
    unsigned int m_errors;

public:

    CoastlineHandlerPass2(const Options& options, Osmium::Output::Base* raw_output, coastline_rings_list_t& clp, OutputDatabase* output) :
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

/* ================================================== */

unsigned int output_rings(coastline_rings_list_t coastline_rings, OutputDatabase& output) {
    unsigned int warnings = 0;

    for (coastline_rings_list_t::const_iterator it = coastline_rings.begin(); it != coastline_rings.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed()) {
            if (cp.npoints() > 3) {
                output.layer_rings()->add(cp.ogr_polygon(), cp.min_way_id(), cp.nways(), cp.npoints(), output.layer_error_points());
            } else if (cp.npoints() == 1) {
                output.add_error(cp.ogr_first_point(), "single_point_in_ring", cp.first_node_id());
                warnings++;
            } else { // cp.npoints() == 2 or 3
                output.add_error(cp.ogr_linestring(), "not_a_ring", cp.min_way_id());
                output.add_error(cp.ogr_first_point(), "not_a_ring", cp.first_node_id());
                output.add_error(cp.ogr_last_point(), "not_a_ring", cp.last_node_id());
                warnings++;
            }
        } else {
            output.add_error(cp.ogr_linestring(), "not_closed", cp.min_way_id());
            output.add_error(cp.ogr_first_point(), "end_point", cp.first_node_id());
            output.add_error(cp.ogr_last_point(), "end_point", cp.last_node_id());
            warnings++;
        }
    }

    return warnings;
}

/* ================================================== */

/**
 * This function assembles all the coastline rings into one huge multipolygon.
 */
OGRMultiPolygon* create_polygons(coastline_rings_list_t coastline_rings) {
    std::vector<OGRGeometry*> all_polygons;
    all_polygons.reserve(coastline_rings.size());

    for (coastline_rings_list_t::const_iterator it = coastline_rings.begin(); it != coastline_rings.end(); ++it) {
        CoastlineRing& cp = **it;
        if (cp.is_closed() && cp.npoints() > 3) { // XXX what's with rings that don't match here?
            all_polygons.push_back(cp.ogr_polygon());
        }
    }

    int is_valid;
    const char* options[] = { "METHOD=ONLY_CCW", NULL };
    OGRGeometry* mega_multipolygon = OGRGeometryFactory::organizePolygons(&all_polygons[0], all_polygons.size(), &is_valid, options);

    assert(mega_multipolygon->getGeometryType() == wkbMultiPolygon);

    return static_cast<OGRMultiPolygon*>(mega_multipolygon);
}

/**
 * Write all polygons in the given multipolygon as polygons to the output database.
 */
void output_polygons(OGRMultiPolygon* multipolygon, OutputDatabase& output) {
    for (int i=0; i < multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(multipolygon->getGeometryRef(i));
        output.layer_polygons()->add(p, p->getExteriorRing()->isClockwise());
    }
}

OGRPolygon* create_rectangular_polygon(double x1, double y1, double x2, double y2, double expand) {
    x1 -= expand;
    x2 += expand;
    y1 -= expand;
    y2 += expand;

    if (x1 < -180) { x1 = -180; }
    if (x1 >  180) { x1 =  180; }
    if (x2 < -180) { x2 = -180; }
    if (x2 >  180) { x2 =  180; }

    if (y1 <  -90) { y1 =  -90; }
    if (y1 >   90) { y2 =   90; }
    if (y2 <  -90) { y2 =  -90; }
    if (y2 >   90) { y2 =   90; }

    OGRLinearRing* ring = new OGRLinearRing();
    ring->addPoint(x1, y1);
    ring->addPoint(x1, y2);
    ring->addPoint(x2, y2);
    ring->addPoint(x2, y1);
    ring->closeRings();

    OGRPolygon* polygon = new OGRPolygon();
    polygon->addRingDirectly(ring);

    return polygon;
}

void split(OGRGeometry* g, OutputDatabase& output, const Options& options) {
    static double expand = 0.0001;
    OGRPolygon* p = static_cast<OGRPolygon*>(g);

    if (p->getExteriorRing()->getNumPoints() <= options.max_points_in_polygons) {
        output.layer_polygons()->add(p, 0); // XXX p->getExteriorRing()->isClockwise());
    } else {
        OGREnvelope envelope;
        p->getEnvelope(&envelope);

        OGRPolygon* b1;
        OGRPolygon* b2;

        if (envelope.MaxX - envelope.MinX < envelope.MaxY-envelope.MinY) {
            // split vertically
            double MidY = (envelope.MaxY+envelope.MinY) / 2;

            b1 = create_rectangular_polygon(envelope.MinX, envelope.MinY, envelope.MaxX, MidY, expand);
            b2 = create_rectangular_polygon(envelope.MinX, MidY, envelope.MaxX, envelope.MaxY, expand);
        } else {
            // split horizontally
            double MidX = (envelope.MaxX+envelope.MinX) / 2;

            b1 = create_rectangular_polygon(envelope.MinX, envelope.MinY, MidX, envelope.MaxY, expand);
            b2 = create_rectangular_polygon(MidX, envelope.MinY, envelope.MaxX, envelope.MaxY, expand);
        }

        OGRGeometry* g1 = p->Intersection(b1);
        OGRGeometry* g2 = p->Intersection(b2);

        if (g1 && (g1->getGeometryType() == wkbPolygon || g1->getGeometryType() == wkbMultiPolygon) &&
            g2 && (g2->getGeometryType() == wkbPolygon || g2->getGeometryType() == wkbMultiPolygon)) {
            if (g1->getGeometryType() == wkbPolygon) {
                split(g1, output, options);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g1);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    split(gp, output, options);
                }
            }
            if (g2->getGeometryType() == wkbPolygon) {
                split(g2, output, options);
            } else {
                OGRMultiPolygon* mp = static_cast<OGRMultiPolygon*>(g2);
                for (int i=0; i < mp->getNumGeometries(); ++i) {
                    OGRPolygon* gp = static_cast<OGRPolygon*>(mp->getGeometryRef(i));
                    split(gp, output, options);
                }
            }
        } else {
            std::cerr << "g1=" << g1 << " g2=" << g2 << "\n";
            if (g1 != 0) {
                std::cerr << "g1 type=" << g1->getGeometryName() << "\n";
            }
            if (g2 != 0) {
                std::cerr << "g2 type=" << g2->getGeometryName() << "\n";
            }
            output.layer_polygons()->add(p, 1);
            delete g2;
            delete g1;
        }

        delete b2;
        delete b1;
    }
}

/* ================================================== */

unsigned int fix_coastline_direction(OGRMultiPolygon* multipolygon, OutputDatabase& output) {
    unsigned int warnings = 0;

    for (int i=0; i < multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(multipolygon->getGeometryRef(i));
        if (!p->getExteriorRing()->isClockwise()) {
            p->getExteriorRing()->reverseWindingOrder();
            OGRLineString* l = static_cast<OGRLineString*>(p->getExteriorRing()->clone());
            output.add_error(l, "direction");
            warnings++;
        }
    }

    return warnings;
}

/* ================================================== */

void output_split_polygons(OGRMultiPolygon* multipolygon, OutputDatabase& output, const Options& options) {
    for (int i=0; i < multipolygon->getNumGeometries(); ++i) {
        OGRPolygon* p = static_cast<OGRPolygon*>(multipolygon->getGeometryRef(i));
        split(p, output, options);
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

    OutputDatabase* output = NULL;
    if (!options.outdb.empty()) { 
        if (options.debug) {
            std::cerr << "Using SRS " << options.epsg << " for output.\n";
            if (options.create_index) {
                std::cerr << "Will create geometry index.\n";
            } else {
                std::cerr << "Will NOT create geometry index.\n";
            }
        }

        output = new OutputDatabase(options.outdb, options.epsg, options.create_index);
    }

    coastline_rings_list_t coastline_rings;

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

    if (output) {
        std::cerr << "-------------------------------------------------------------------------------\n";
        std::cerr << "Create and output polygons...\n";
        t = time(NULL);
        if (options.output_rings) {
            warnings += output_rings(coastline_rings, *output);
        }
        if (options.output_polygons) {
            OGRMultiPolygon* mp = create_polygons(coastline_rings);
            warnings += fix_coastline_direction(mp, *output);
            if (options.split_large_polygons) {
                output_split_polygons(mp, *output, options);
            } else {
                output_polygons(mp, *output);
            }
        }
        std::cerr << "Done (about " << (time(NULL)-t)/60 << " minutes).\n";
        print_memory_usage();
    }

    std::cerr << "-------------------------------------------------------------------------------\n";

    delete output;
    delete raw_output;
    delete outfile;

    if (warnings) {
        std::cerr << "There were " << warnings << " warnings.\n";
    }
    if (errors) {
        std::cerr << "There were " << errors << " errors.\n";
    }
    if (errors) {
        return return_code_error;
    } else if (warnings) {
        return return_code_warning;
    } else {
        return return_code_ok;
    }
}

