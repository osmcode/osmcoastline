/*

  Copyright 2012-2019 Jochen Topf <jochen@topf.org>.

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
  along with OSMCoastline.  If not, see <https://www.gnu.org/licenses/>.

*/

#include "coastline_polygons.hpp"
#include "coastline_ring_collection.hpp"
#include "options.hpp"
#include "output_database.hpp"
#include "return_codes.hpp"
#include "srs.hpp"
#include "stats.hpp"
#include "util.hpp"
#include "version.hpp"

#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include <ogr_core.h>
#include <ogr_geometry.h>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef _MSC_VER
# include <unistd.h>
#else
# include <io.h>
#endif

// The global SRS object is used in many places to transform
// from WGS84 to the output SRS etc.
SRS srs;

// Global debug marker
bool debug;

// If there are more than this many warnings, the program exit code will indicate an error.
const unsigned int max_warnings = 500;

/* ================================================== */

/**
 * This function assembles all the coastline rings into one huge multipolygon.
 */
polygon_vector_type create_polygons(CoastlineRingCollection& coastline_rings, OutputDatabase& output, unsigned int* warnings, unsigned int* errors) {
    std::vector<OGRGeometry*> all_polygons = coastline_rings.add_polygons_to_vector();

    if (all_polygons.empty()) {
        throw std::runtime_error{"No polygons created!"};
    }

    int is_valid = false;
    const char* options[] = {"METHOD=ONLY_CCW", nullptr};
    if (debug) {
        std::cerr << "Calling organizePolygons()\n";
    }
    std::unique_ptr<OGRGeometry> mega_geometry{OGRGeometryFactory::organizePolygons(&all_polygons[0], all_polygons.size(), &is_valid, options)};
    if (debug) {
        std::cerr << "organizePolygons() done (" << (is_valid ? "valid" : "invalid") << ")\n";
    }

    if (!mega_geometry) {
        throw std::runtime_error{"No polygons created!"};
    }

    polygon_vector_type polygons;

    if (mega_geometry->getGeometryType() == wkbPolygon) {
        if (mega_geometry->IsValid()) {
            polygons.push_back(static_cast_unique_ptr<OGRPolygon>(std::move(mega_geometry)));
        } else {
            std::cerr << "Ignoring invalid polygon geometry.\n";
            (*errors)++;
        }
    } else if (mega_geometry->getGeometryType() != wkbMultiPolygon) {
        throw std::runtime_error{"mega geometry isn't a (multi)polygon. Something is very wrong!"};
    } else {

        // This isn't an owning pointer on purpose. We are going to "steal" parts
        // of the geometry a few lines below but only mark them as unowned farther
        // below when we are calling removeGeometry() on it. If this was an
        // owning pointer we'd get a double free if there is an error.
        auto* mega_multipolygon = static_cast<OGRMultiPolygon*>(mega_geometry.release());

        polygons.reserve(mega_multipolygon->getNumGeometries());
        for (int i = 0; i < mega_multipolygon->getNumGeometries(); ++i) {
            OGRGeometry* geom = mega_multipolygon->getGeometryRef(i);
            assert(geom);
            assert(geom->getGeometryType() == wkbPolygon);
            std::unique_ptr<OGRPolygon> p{static_cast<OGRPolygon*>(geom)};
            if (p->IsValid()) {
                polygons.push_back(std::move(p));
            } else {
                output.add_error_line(make_unique_ptr_clone<OGRLineString>(p->getExteriorRing()), "invalid");
                std::unique_ptr<OGRGeometry> buf0{p->Buffer(0)};
                if (buf0 && buf0->getGeometryType() == wkbPolygon && buf0->IsValid()) {
                    buf0->assignSpatialReference(srs.wgs84());
                    polygons.push_back(static_cast_unique_ptr<OGRPolygon>(std::move(buf0)));
                    (*warnings)++;
                } else {
                    std::cerr << "Ignoring invalid polygon geometry.\n";
                    (*errors)++;
                }
            }
        }

        mega_multipolygon->removeGeometry(-1, FALSE);
        delete mega_multipolygon; // NOLINT(cppcoreguidelines-owning-memory)
    }

    return polygons;
}


/* ================================================== */

std::string memory_usage() {
    osmium::MemoryUsage mem;
    std::ostringstream s;
    s << "Memory used: current: " << mem.current() << " MBytes, peak: " << mem.peak() << " MBytes\n";
    return s.str();
}

/* ================================================== */

int main(int argc, char *argv[]) {
    Stats stats{};
    unsigned int warnings = 0;
    unsigned int errors = 0;

    // Parse command line and setup 'options' object with them.
    Options options{argc, argv};

    // The vout object is an output stream we can write to instead of
    // std::cerr. Nothing is written if we are not in verbose mode.
    // The running time will be prepended to output lines.
    osmium::util::VerboseOutput vout{options.verbose};

    debug = options.debug;

    vout << "Started osmcoastline " << get_osmcoastline_long_version() << " / " << get_libosmium_version() << '\n';

    CPLSetConfigOption("OGR_ENABLE_PARTIAL_REPROJECTION", "TRUE");
    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");
    vout << "Using SRS " << options.epsg << " for output. (Change with the --srs/s option.)\n";
    if (!srs.set_output(options.epsg)) {
        std::cerr << "Setting up output transformation failed\n";
        std::exit(return_code_fatal);
    }

    // Optionally set up segments file
    int segments_fd = -1;
    if (!options.segmentfile.empty()) {
        vout << "Writing segments to file '" << options.segmentfile << "' (because you told me to with --write-segments/-S option).\n";
        segments_fd = ::open(options.segmentfile.c_str(), O_WRONLY | O_CREAT, 0666); // NOLINT(hicpp-signed-bitwise)
        if (segments_fd == -1) {
            std::cerr << "Couldn't open file '" << options.segmentfile << "' (" << std::strerror(errno) << ")\n";
            std::exit(return_code_fatal);
        }
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
    OutputDatabase output_database{options.output_database, srs, options.create_index};

    // The collection of all coastline rings we will be filling and then
    // operating on.
    CoastlineRingCollection coastline_rings;

    try {
        // This is in an extra scope so that the considerable amounts of memory
        // held by some intermediate datastructures is recovered after we don't
        // need them any more.
        vout << "Reading from file '" << options.inputfile << "'.\n";
        osmium::io::File infile{options.inputfile};

        vout << "Reading ways (1st pass through input file)...\n";
        osmium::io::Reader reader1{infile, osmium::osm_entity_bits::way};
        while (const auto buffer = reader1.read()) {
            for (const auto& way : buffer.select<osmium::Way>()) {
                // We are only interested in ways tagged with natural=coastline.
                if (way.tags().has_tag("natural", "coastline")) {
                    // ignore bogus coastline in Antarctica
                    if (!way.tags().has_tag("coastline", "bogus")) {
                        coastline_rings.add_way(way);
                    }
                }
            }
        }
        reader1.close();
        stats.ways = coastline_rings.num_ways();
        stats.unconnected_nodes = coastline_rings.num_unconnected_nodes();
        stats.rings = coastline_rings.size();
        stats.rings_from_single_way = coastline_rings.num_rings_from_single_way();
        vout << "  There are "
             << coastline_rings.num_unconnected_nodes()
             << " nodes where the coastline is not closed.\n";
        vout << "  There are "
             << coastline_rings.size()
             << " coastline rings ("
             << coastline_rings.num_rings_from_single_way()
             << " from a single closed way and "
             << (coastline_rings.size() - coastline_rings.num_rings_from_single_way())
             << " others).\n";
        vout << memory_usage();

        vout << "Reading nodes (2nd pass through input file)...\n";
        locmap_type locmap{};
        coastline_rings.setup_locations(locmap);
        osmium::geom::OGRFactory<> factory;
        osmium::io::Reader reader2{infile, osmium::osm_entity_bits::node};
        while (const auto buffer = reader2.read()) {
            for (const auto& node : buffer.select<osmium::Node>()) {
                if (node.tags().has_tag("natural", "coastline")) {
                    try {
                        output_database.add_error_point(factory.create_point(node), "tagged_node", node.id());
                    } catch (const osmium::geometry_error&) {
                        std::cerr << "Ignoring illegal geometry for node " << node.id() << ".\n";
                    }
                }

                const auto ret = locmap.equal_range(node.id());
                for (auto it = ret.first; it != ret.second; ++it) {
                    *(it->second) = node.location();
                }
            }
        }
        reader2.close();
    } catch (const std::exception& e) {
        vout << e.what() << '\n';
        std::exit(return_code_fatal);
    }

    vout << "Checking for missing locations...\n";
    unsigned int missing_locations = coastline_rings.check_locations(options.debug);
    if (missing_locations) {
        vout << "  There are " << missing_locations << " locations missing. Check that input file contains all nodes needed.\n";
        std::exit(return_code_error);
    } else {
        vout << "  All locations are there.\n";
    }

    vout << memory_usage();

    output_database.set_options(options);

    vout << "Check line segments for intersections and overlaps...\n";
    warnings += coastline_rings.check_for_intersections(output_database, segments_fd);

    if (segments_fd != -1) {
        ::close(segments_fd);
    }

    vout << "Trying to close Antarctica ring...\n";
    if (coastline_rings.close_antarctica_ring(options.epsg)) {
        vout << "  Closed Antarctica ring.\n";
    } else {
        vout << "  Did not find open Antarctica ring.\n";
    }

    if (options.close_rings) {
        vout << "Close broken rings... (Use --close-distance/-c 0 if you do not want this.)\n";
        vout << "  Closing if distance between nodes smaller than " << options.close_distance << ". (Set this with --close-distance/-c.)\n";
        coastline_rings.close_rings(output_database, options.debug, options.close_distance);
        stats.rings_fixed = coastline_rings.num_fixed_rings();
        errors += coastline_rings.num_fixed_rings();
        vout << "  Closed " << coastline_rings.num_fixed_rings() << " rings. This left "
             << coastline_rings.num_unconnected_nodes() << " nodes where the coastline could not be closed.\n";
        errors += coastline_rings.num_unconnected_nodes();
    } else {
        vout << "Not closing broken rings (because you used the option --close-distance/-c 0).\n";
    }

    if (options.output_rings) {
        vout << "Writing out rings... (Because you gave the --output-rings/-r option.)\n";
        warnings += coastline_rings.output_rings(output_database);
    } else {
        vout << "Not writing out rings. (Use option --output-rings/-r if you want the rings.)\n";
    }

    if (options.output_polygons != output_polygon_type::none || options.output_lines) {
        try {
            vout << "Create polygons...\n";
            CoastlinePolygons coastline_polygons{create_polygons(coastline_rings, output_database, &warnings, &errors), \
                                                 output_database, \
                                                 options.bbox_overlap, \
                                                 options.max_points_in_polygon};

            stats.land_polygons_before_split = coastline_polygons.num_polygons();

            vout << "Fixing coastlines going the wrong way...\n";
            stats.rings_turned_around = coastline_polygons.fix_direction();
            vout << "  Turned " << stats.rings_turned_around << " polygons around.\n";
            warnings += stats.rings_turned_around;

            if (options.epsg != 4326) {
                vout << "Transforming polygons to EPSG " << options.epsg << "...\n";
                coastline_polygons.transform();
            }

            if (options.output_lines) {
                vout << "Writing coastlines as lines... (Because you used --output-lines/-l)\n";
                coastline_polygons.output_lines(options.max_points_in_polygon);
            } else {
                vout << "Not writing coastlines as lines (Use --output-lines/-l if you want this).\n";
            }

            if (options.output_polygons != output_polygon_type::none) {
                if (options.epsg == 4326) {
                    vout << "Checking for questionable input data...\n";
                    const unsigned int questionable = coastline_rings.output_questionable(coastline_polygons, output_database);
                    warnings += questionable;
                    vout << "  Found " << questionable << " rings in input data.\n";
                } else {
                    vout << "Not performing check for questionable input data, because it only works in EPSG:4326...\n";
                }

                if (options.split_large_polygons) {
                    vout << "Split polygons with more than " << options.max_points_in_polygon << " points... (Use --max-points/-m to change this. Set to 0 not to split at all.)\n";
                    vout << "  Using overlap of " << options.bbox_overlap << " (Set this with --bbox-overlap/-b).\n";
                    coastline_polygons.split();
                    stats.land_polygons_after_split = coastline_polygons.num_polygons();
                }

                vout << "Checking and making polygons valid...\n";
                warnings += coastline_polygons.check_polygons();

                if (options.output_polygons == output_polygon_type::land ||
                    options.output_polygons == output_polygon_type::both) {
                    vout << "Writing out land polygons...\n";
                    coastline_polygons.output_land_polygons(options.output_polygons == output_polygon_type::both);
                }
                if (options.output_polygons == output_polygon_type::water ||
                    options.output_polygons == output_polygon_type::both) {
                    vout << "Writing out water polygons...\n";
                    coastline_polygons.output_water_polygons();
                }
            }
        } catch (const std::runtime_error& e) {
            vout << e.what() << '\n';
            ++errors;
        }
    } else {
        vout << "Not creating polygons (Because you used the --output-polygons=none option).\n";
    }

    vout << memory_usage();

    vout << "Committing database transactions...\n";
    output_database.set_meta(vout.runtime(), osmium::MemoryUsage{}.peak(), stats);
    output_database.commit();
    vout << "All done.\n";
    vout << memory_usage();

    std::cout << "There were " << warnings << " warnings.\n";
    std::cout << "There were " << errors << " errors.\n";

    if (errors || warnings > max_warnings) {
        return return_code_error;
    }

    if (warnings) {
        return return_code_warning;
    }

    return return_code_ok;
}

