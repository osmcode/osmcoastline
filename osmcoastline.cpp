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
#include "coastline_ring_collection.hpp"
#include "output_database.hpp"
#include "output_layers.hpp"

#include "options.hpp"
#include "coastline_handlers.hpp"

/* ================================================== */

/**
 * This function assembles all the coastline rings into one huge multipolygon.
 */
OGRMultiPolygon* create_polygons(CoastlineRingCollection coastline_rings) {
    std::vector<OGRGeometry*> all_polygons;
    coastline_rings.add_polygons_to_vector(all_polygons);

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
    static double expand = options.bbox_overlap;
    OGRPolygon* p = static_cast<OGRPolygon*>(g);

    if (p->getExteriorRing()->getNumPoints() <= options.max_points_in_polygon) {
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
        if (p->IsValid()) {
            split(p, output, options);
        } else {
            OGRPolygon* pb = dynamic_cast<OGRPolygon*>(p->Buffer(0));
            if (pb) {
                std::cerr << "not valid, but buffer ok\n";
                split(pb, output, options);
            } else {
                std::cerr << "not valid, and buffer not polygon\n";
            }
        }
    }
}

/* ================================================== */

std::pair<int, int> get_memory_usage() {
    char filename[100];
    sprintf(filename, "/proc/%d/status", getpid());
    std::ifstream status_file(filename);
    std::string line;

    int vmpeak = 0;
    int vmsize = 0;
    if (status_file.is_open()) {
        while (! status_file.eof() ) {
            std::getline(status_file, line);
            if (line.substr(0, 6) == "VmPeak") {
                int f = line.find_first_of("0123456789");
                int l = line.find_last_of("0123456789");
                vmpeak = atoi(line.substr(f, l-f+1).c_str());
            }
            if (line.substr(0, 6) == "VmSize") {
                int f = line.find_first_of("0123456789");
                int l = line.find_last_of("0123456789");
                vmsize = atoi(line.substr(f, l-f+1).c_str());
            }
        }
        status_file.close();
    }

    return std::make_pair(vmsize / 1024, vmpeak / 1024);
}

std::string memory_usage() {
    std::pair<int, int> mem = get_memory_usage();
    std::ostringstream s;
    s << "Memory used currently: " << mem.first << " MB (Peak was: " << mem.second << " MB).\n";
    return s.str();
}

/* ================================================== */

class VerboseOutput {

    time_t m_start;
    bool m_verbose;
    bool m_newline;

public:

    VerboseOutput(bool verbose) :
        m_start(time(NULL)),
        m_verbose(verbose),
        m_newline(true)
    { }

    int runtime() const {
        return time(NULL) - m_start;
    }

    void start_line() {
        if (m_newline) {
            int elapsed = time(NULL) - m_start;
            char timestr[20];
            snprintf(timestr, sizeof(timestr)-1, "[%2d:%02d] ", elapsed / 60, elapsed % 60);
            std::cerr << timestr;
            m_newline = false;
        }
    }

    template<typename T>
    friend VerboseOutput& operator<<(VerboseOutput& out, T t) {
        if (out.m_verbose) {
            std::ostringstream o;
            o << t;
            out.start_line();
            std::cerr << o.str();
            if (o.str()[o.str().size()-1] == '\n') {
                out.m_newline = true;
            }
        }
        return out;
    }

};

/* ================================================== */

int main(int argc, char *argv[]) {
    unsigned int warnings = 0;
    unsigned int errors = 0;

    Options options(argc, argv);

    VerboseOutput vout(options.verbose);

    Osmium::init(options.debug);

    Osmium::OSMFile* outfile = NULL;
    Osmium::Output::Base* raw_output = NULL;
    if (options.output_osm.empty()) {
        vout << "Not writing raw OSM output. (Add --output-osm/-O if you want this.)\n";
    } else {
        vout << "Writing raw output to file '" << options.output_osm << "'. (Was set with --output-osm/-O option.)\n";
        outfile = new Osmium::OSMFile(options.output_osm);
        raw_output = outfile->create_output_file();
    }

    OutputDatabase* output = NULL;
    if (options.output_database.empty()) { 
        vout << "Not writing output database (because you did not give the --output-database/-o option).\n";
    } else {
        vout << "Writing to output database '" << options.output_database << "'. (Was set with the --output-database/-o option.)\n";
        vout << "Using SRS " << options.epsg << " for output. (Change with the --srs/s option.)\n";
        if (options.create_index) {
            vout << "Will create geometry index (because you told me to using --create-index/-i).\n";
        } else {
            vout << "Will NOT create geometry index. (If you want an index use --create-index/-i.)\n";
        }
        output = new OutputDatabase(options.output_database, options.epsg, options.create_index);
    }

    CoastlineRingCollection coastline_rings;

    {
        // This is in an extra scope so that the considerable amounts of memory
        // held by the handlers is recovered after we don't need them any more.
        vout << "Reading from file '" << options.inputfile << "'.\n";
        Osmium::OSMFile infile(options.inputfile);

        vout << "Reading ways (1st pass through input file)...\n";
        CoastlineHandlerPass1 handler_pass1(raw_output, coastline_rings);
        infile.read(handler_pass1);
        vout << "  There are " << coastline_rings.num_unconnected_nodes() << " nodes where the coastline is not closed.\n";
        vout << "  There are " << coastline_rings.size() << " coastline rings ("
             << coastline_rings.num_rings_from_single_way() << " from a single way and "
             << coastline_rings.size() - coastline_rings.num_rings_from_single_way() << " from multiple ways).\n";
        vout << memory_usage();

        vout << "Reading nodes (2nd pass through input file)...\n";
        CoastlineHandlerPass2 handler_pass2(raw_output, coastline_rings, output);
        infile.read(handler_pass2);
    }

    vout << memory_usage();

    if (options.close_rings) {
        vout << "Close broken rings... (Use --close-distance/-c 0 if you do not want this.)\n";
        vout << "  Closing if distance between nodes smaller than " << options.close_distance << ". (Set this with --close-distance/-c.)\n";
        coastline_rings.close_rings(*output, options.debug, options.close_distance);
        vout << "  Closed " << coastline_rings.num_fixed_rings() << " rings. This left "
             << coastline_rings.num_unconnected_nodes() << " nodes where the coastline could not be closed.\n";
    } else {
        vout << "Not closing broken rings (because you used the option --close-distance/-c 0).\n";
    }

    if (output) {
        if (options.output_rings) {
            vout << "Writing out rings... (Because you gave the --output-rings/-r option.)\n";
            warnings += coastline_rings.output_rings(*output);
        } else {
            vout << "Not writing out rings. (Use option --output-rings/-r if you want the rings.)\n";
        }

        if (options.output_polygons) {
            vout << "Create polygons...\n";
            OGRMultiPolygon* mp = create_polygons(coastline_rings);

            vout << "Fixing coastlines going the wrong way...\n";
            int fixed = fix_coastline_direction(mp, *output);
            vout << "  Fixed orientation of " << fixed << " polygons.\n";
            warnings += fixed;

            if (options.split_large_polygons) {
                vout << "Split polygons with more than " << options.max_points_in_polygon << " points... (Use --max-points/-m to change this. Set to 0 not to split at all.)\n";
                vout << "  Using overlap of " << options.bbox_overlap << " (Set this with --bbox-overlap/-b).\n";
                output_split_polygons(mp, *output, options);
            } else {
                vout << "Writing out complete polygons... (Because you set --max-points/-m to 0.)\n";
                output_polygons(mp, *output);
            }
        } else {
            vout << "Not creating polygons (Because you set the --no-polygons/-p option).\n";
        }
    }

    vout << memory_usage();

    if (output) {
        vout << "Commiting database transactions...\n";
        output->set_meta(vout.runtime(), get_memory_usage().second);
        delete output;
    }

    delete raw_output;
    delete outfile;

    vout << "All done.\n";
    vout << memory_usage();

    vout << "There were " << warnings << " warnings.\n";
    vout << "There were " << errors << " errors.\n";

    if (errors) {
        return return_code_error;
    } else if (warnings) {
        return return_code_warning;
    } else {
        return return_code_ok;
    }
}

