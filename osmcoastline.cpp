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

#include <fstream>
#include <list>
#include <time.h>
#include <unistd.h>

#include <osmium.hpp>
#include <osmium/geometry/point.hpp>

#include "osmcoastline.hpp"
#include "coastline_ring.hpp"
#include "coastline_ring_collection.hpp"
#include "coastline_polygons.hpp"
#include "output_database.hpp"
#include "output_layers.hpp"

#include "options.hpp"
#include "stats.hpp"
#include "coastline_handlers.hpp"
#include "srs.hpp"

// The global SRS object is used in many places to transform
// from WGS84 to the output SRS etc.
SRS srs;

// Global debug marker
bool debug;

/* ================================================== */

/**
 * This function assembles all the coastline rings into one huge multipolygon.
 */
polygon_vector_t* create_polygons(CoastlineRingCollection coastline_rings, OutputDatabase& output) {
    std::vector<OGRGeometry*> all_polygons;
    coastline_rings.add_polygons_to_vector(all_polygons);

    int is_valid;
    const char* options[] = { "METHOD=ONLY_CCW", NULL };
    OGRGeometry* mega_multipolygon = OGRGeometryFactory::organizePolygons(&all_polygons[0], all_polygons.size(), &is_valid, options);

    assert(mega_multipolygon->getGeometryType() == wkbMultiPolygon);

    polygon_vector_t* polygons = new polygon_vector_t;
    polygons->reserve(static_cast<OGRMultiPolygon*>(mega_multipolygon)->getNumGeometries());
    for (int i=0; i < static_cast<OGRMultiPolygon*>(mega_multipolygon)->getNumGeometries(); ++i) {
        OGRGeometry* geom = static_cast<OGRMultiPolygon*>(mega_multipolygon)->getGeometryRef(i);
        assert(geom->getGeometryType() == wkbPolygon);
        OGRPolygon* p = static_cast<OGRPolygon*>(geom);
        if (p->IsValid()) {
            polygons->push_back(p);
        } else {
            output.add_error_line(static_cast<OGRLineString*>(p->getExteriorRing()->clone()), "invalid");
            OGRGeometry* buf0 = p->Buffer(0);
            if (buf0 && buf0->getGeometryType() == wkbPolygon && buf0->IsValid()) {
                buf0->assignSpatialReference(srs.wgs84());
                polygons->push_back(static_cast<OGRPolygon*>(buf0));
            } else {
                std::cerr << "Ignoring invalid polygon geometry.\n";
                delete buf0;
            }
            delete p;
        }
    }

    static_cast<OGRMultiPolygon*>(mega_multipolygon)->removeGeometry(-1, FALSE);

    return polygons;
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
    Stats stats;
    unsigned int warnings = 0;
    unsigned int errors = 0;

    // Parse command line and setup 'options' object with them.
    Options options(argc, argv);

    // The vout object is an output stream we can write to instead of
    // std::cerr. Nothing is written if we are not in verbose mode.
    // The running time will be prepended to output lines.
    VerboseOutput vout(options.verbose);

    // Initialize Osmium.
    Osmium::init(options.debug);
    debug = options.debug;

    vout << "Using SRS " << options.epsg << " for output. (Change with the --srs/s option.)\n";
    if (!srs.set_output(options.epsg)) {
        std::cerr << "Setting up output transformation failed\n";
        exit(return_code_fatal);
    }

    // Set up output database.
    vout << "Writing to output database '" << options.output_database << "'. (Was set with the --output-database/-o option.)\n";
    if (options.overwrite_output) {
        vout << "Removing database output file (if it exists) (because you told me to with --overwrite/-f).\n";
        unlink(options.output_database.c_str());
    }
    if (options.create_index) {
        vout << "Will create geometry index. (If you do not want an index use --no-index/-i.)\n";
    } else {
        vout << "Will NOT create geometry index (because you told me to using --no-index/-i).\n";
    }
    OutputDatabase output_database(options.output_database, options.create_index);

    // The collection of all coastline rings we will be filling and then
    // operating on.
    CoastlineRingCollection coastline_rings;

    {
        // This is in an extra scope so that the considerable amounts of memory
        // held by the handlers is recovered after we don't need them any more.
        vout << "Reading from file '" << options.inputfile << "'.\n";
        Osmium::OSMFile infile(options.inputfile);

        vout << "Reading ways (1st pass through input file)...\n";
        CoastlineHandlerPass1 handler_pass1(coastline_rings);
        infile.read(handler_pass1);
        stats.ways = coastline_rings.num_ways();
        stats.unconnected_nodes = coastline_rings.num_unconnected_nodes();
        stats.rings = coastline_rings.size();
        stats.rings_from_single_way = coastline_rings.num_rings_from_single_way();
        vout << "  There are " << coastline_rings.num_unconnected_nodes() << " nodes where the coastline is not closed.\n";
        vout << "  There are " << coastline_rings.size() << " coastline rings ("
             << coastline_rings.num_rings_from_single_way() << " from a single way and "
             << coastline_rings.size() - coastline_rings.num_rings_from_single_way() << " from multiple ways).\n";
        vout << memory_usage();

        vout << "Reading nodes (2nd pass through input file)...\n";
        CoastlineHandlerPass2 handler_pass2(coastline_rings, output_database);
        infile.read(handler_pass2);
    }

    vout << memory_usage();

    output_database.set_options(options);

    if (options.close_rings) {
        vout << "Close broken rings... (Use --close-distance/-c 0 if you do not want this.)\n";
        vout << "  Closing if distance between nodes smaller than " << options.close_distance << ". (Set this with --close-distance/-c.)\n";
        coastline_rings.close_rings(output_database, options.debug, options.close_distance);
        stats.rings_fixed = coastline_rings.num_fixed_rings();
        vout << "  Closed " << coastline_rings.num_fixed_rings() << " rings. This left "
            << coastline_rings.num_unconnected_nodes() << " nodes where the coastline could not be closed.\n";
    } else {
        vout << "Not closing broken rings (because you used the option --close-distance/-c 0).\n";
    }

    if (options.output_rings) {
        vout << "Writing out rings... (Because you gave the --output-rings/-r option.)\n";
        warnings += coastline_rings.output_rings(output_database);
    } else {
        vout << "Not writing out rings. (Use option --output-rings/-r if you want the rings.)\n";
    }

    if (options.output_polygons) {
        vout << "Create polygons...\n";
        CoastlinePolygons coastline_polygons(create_polygons(coastline_rings, output_database), output_database, options.bbox_overlap, options.max_points_in_polygon);
        stats.land_polygons_before_split = coastline_polygons.num_polygons();

        vout << "Fixing coastlines going the wrong way...\n";
        stats.rings_turned_around = coastline_polygons.fix_direction();
        vout << "  Turned " << stats.rings_turned_around << " polygons around.\n";
        warnings += stats.rings_turned_around;

        if (options.epsg != 4326) {
            vout << "Transforming polygons to EPSG " << options.epsg << "...\n";
            coastline_polygons.transform();
        }

        if (options.simplify) {
            vout << "Simplifying polygons with tolerance " << options.tolerance << " (because you used --simplify/-S).\n";
            coastline_polygons.simplify(options.tolerance);
        }

        if (options.split_large_polygons || options.water) {
            vout << "Split polygons with more than " << options.max_points_in_polygon << " points... (Use --max-points/-m to change this. Set to 0 not to split at all.)\n";
            vout << "  Using overlap of " << options.bbox_overlap << " (Set this with --bbox-overlap/-b).\n";
            coastline_polygons.split();
            stats.land_polygons_after_split = coastline_polygons.num_polygons();
        }
        if (options.water) {
            vout << "Writing out water polygons...\n";
            coastline_polygons.output_water_polygons();
        } else {
            coastline_polygons.output_land_polygons();
        }
    } else {
        vout << "Not creating polygons (Because you set the --no-polygons/-p option).\n";
    }

    vout << memory_usage();

    vout << "Commiting database transactions...\n";
    output_database.set_meta(vout.runtime(), get_memory_usage().second, stats);
    output_database.commit();
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

