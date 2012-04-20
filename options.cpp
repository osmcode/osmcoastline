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
#include <string>
#include <cstring>
#include <cstdlib>
#include <getopt.h>

#include "osmcoastline.hpp"
#include "options.hpp"

Options::Options(int argc, char* argv[]) :
    inputfile(),
    bbox_overlap(-1),
    close_distance(1.0),
    close_rings(true),
    create_index(true),
    debug(false),
    max_points_in_polygon(1000),
    split_large_polygons(true),
    output_polygons(true),
    output_database(),
    output_osm(),
    overwrite_output(false),
    output_rings(false),
    epsg(4326),
    simplify(false),
    tolerance(0),
    verbose(false),
    water(false)
{
    static struct option long_options[] = {
        {"bbox-overlap",    required_argument, 0, 'b'},
        {"close-distance",  required_argument, 0, 'c'},
        {"no-index",              no_argument, 0, 'i'},
        {"debug",                 no_argument, 0, 'd'},
        {"help",                  no_argument, 0, 'h'},
        {"max-points",      required_argument, 0, 'm'},
        {"no-polygons",           no_argument, 0, 'p'},
        {"output-database", required_argument, 0, 'o'},
        {"output-osm",      required_argument, 0, 'O'},
        {"output-rings",          no_argument, 0, 'r'},
        {"overwrite",             no_argument, 0, 'f'},
        {"srs",             required_argument, 0, 's'},
#ifdef EXPERIMENTAL
        {"simplify",        required_argument, 0, 'S'},
#endif
        {"verbose",               no_argument, 0, 'v'},
        {"water",                 no_argument, 0, 'w'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "b:c:idhm:po:O:rfs:S:vw", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
            case 'b':
                bbox_overlap = atof(optarg);
                break;
            case 'c':
                close_distance = atoi(optarg);
                if (close_distance == 0) {
                    close_rings = false;
                }
                break;
            case 'i':
                create_index = false;
                break;
            case 'd':
                debug = true;
                std::cerr << "Enabled debug option\n";
                break;
            case 'h':
                print_help();
                exit(return_code_ok);
            case 'm':
                max_points_in_polygon = atoi(optarg);
                if (max_points_in_polygon == 0) {
                    split_large_polygons = false;
                }
                break;
            case 'p':
                output_polygons = false;
                break;
            case 'o':
                output_database = optarg;
                break;
            case 'O':
                output_osm = optarg;
                break;
            case 'r':
                output_rings = true;
                break;
            case 'f':
                overwrite_output = true;
                break;
            case 's':
                epsg = get_epsg(optarg);
                break;
#ifdef EXPERIMENTAL
            case 'S':
                simplify = true;
                tolerance = atof(optarg);
                break;
#endif
            case 'v':
                verbose = true;
                break;
            case 'w':
                water = true;
                break;
            default:
                exit(return_code_cmdline);
        }
    }

    if (!split_large_polygons && water) {
        std::cerr << "Options -w/--water and -m/--max-points=0 are mutually exclusive\n";
        exit(return_code_cmdline);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        exit(return_code_cmdline);
    }

    if (output_database.empty() && output_osm.empty()) {
        std::cerr << "You have to give one or both of the --output-database/-o and --output-osm/-O options.\n";
        exit(return_code_cmdline);
    }

    if (bbox_overlap == -1) {
        if (epsg == 4326) {
            bbox_overlap = 0.0001;
        } else {
            bbox_overlap = 10;
        }
    }

    inputfile = argv[optind];
}

int Options::get_epsg(const char* text) {
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

void Options::print_help() {
    std::cout << "osmcoastline [OPTIONS] OSMFILE\n"
                << "Options:\n"
                << "  -h, --help                 - This help message\n"
                << "  -c, --close-distance       - Distance between nodes under which open rings\n"
                << "                               are closed (0 - disable closing of rings)\n"
                << "  -b, --bbox-overlap         - Set overlap when splitting polygons\n"
                << "  -i, --no-index             - Do not create spatial indexes in output database\n"
                << "  -d, --debug                - Enable debugging output\n"
                << "  -f, --overwrite            - Overwrite output files if they already exist\n"
                << "  -m, --max-points           - Split polygons with more than this many points\n"
                << "                               (0 - disable splitting)\n"
                << "  -p, --no-polygons          - Do not create polygons\n"
                << "  -o, --output-database=FILE - Spatialite database file for output\n"
                << "  -O, --output-osm=FILE      - Write raw OSM output to file\n"
                << "  -r, --output-rings         - Output rings to database file\n"
                << "  -s, --srs=EPSGCODE         - Set SRS (4326 for WGS84 (default) or 3857)\n"
#ifdef EXPERIMENTAL
                << "  -S, --simplify=TOLERANCE   - Simplify coastline with given tolerance\n"
#endif
                << "  -v, --verbose              - Verbose output\n"
                << "  -w, --water                - Create water polygons instead of land polygons\n"
                << "\n"
                << "At least one of --output-database/-o and --output-osm/-O is needed.\n"
    ;
}

