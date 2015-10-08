/*

  Copyright 2012-2015 Jochen Topf <jochen@topf.org>.

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
    output_polygons(output_polygon_type::land),
    output_database(),
    overwrite_output(false),
    output_rings(false),
    output_lines(false),
    epsg(4326),
    simplify(false),
    tolerance(0),
    verbose(false),
    segmentfile() {
    static struct option long_options[] = {
        {"bbox-overlap",    required_argument, 0, 'b'},
        {"close-distance",  required_argument, 0, 'c'},
        {"no-index",              no_argument, 0, 'i'},
        {"debug",                 no_argument, 0, 'd'},
        {"help",                  no_argument, 0, 'h'},
        {"output-lines",          no_argument, 0, 'l'},
        {"max-points",      required_argument, 0, 'm'},
        {"output-database", required_argument, 0, 'o'},
        {"output-polygons", required_argument, 0, 'p'},
        {"output-rings",          no_argument, 0, 'r'},
        {"overwrite",             no_argument, 0, 'f'},
        {"srs",             required_argument, 0, 's'},
        {"write-segments",  required_argument, 0, 'S'},
        {"verbose",               no_argument, 0, 'v'},
        {"version",               no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "b:c:idhlm:o:p:rfs:S:vV", long_options, 0);
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
            case 'l':
                output_lines = true;
                break;
            case 'm':
                max_points_in_polygon = atoi(optarg);
                if (max_points_in_polygon == 0) {
                    split_large_polygons = false;
                }
                break;
            case 'p':
                if (!strcmp(optarg, "none")) {
                    output_polygons = output_polygon_type::none;
                } else if (!strcmp(optarg, "land")) {
                    output_polygons = output_polygon_type::land;
                } else if (!strcmp(optarg, "water")) {
                    output_polygons = output_polygon_type::water;
                } else if (!strcmp(optarg, "both")) {
                    output_polygons = output_polygon_type::both;
                } else {
                    std::cerr << "Unknown argument '" << optarg << "' for -p/--output-polygon option\n";
                    exit(return_code_cmdline);
                }
                break;
            case 'o':
                output_database = optarg;
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
            case 'S':
                segmentfile = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'V':
                std::cout << "osmcoastline version " OSMCOASTLINE_VERSION "\n"
                          << "Copyright (C) 2012-2015  Jochen Topf <jochen@topf.org>\n"
                          << "License: GNU GENERAL PUBLIC LICENSE Version 3 <http://gnu.org/licenses/gpl.html>.\n"
                          << "This is free software: you are free to change and redistribute it.\n"
                          << "There is NO WARRANTY, to the extent permitted by law.\n";
                exit(return_code_ok);
            default:
                exit(return_code_cmdline);
        }
    }

    if (!split_large_polygons && (output_polygons == output_polygon_type::water || output_polygons == output_polygon_type::both)) {
        std::cerr << "Can not use -m/--max-points=0 when writing out water polygons\n";
        exit(return_code_cmdline);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE\n";
        exit(return_code_cmdline);
    }

    if (output_database.empty()) {
        std::cerr << "Missing --output-database/-o option.\n";
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

void Options::print_help() const {
    std::cout << "osmcoastline [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help                 - This help message\n"
              << "  -c, --close-distance=DIST  - Distance between nodes under which open rings\n"
              << "                               are closed (0 - disable closing of rings)\n"
              << "  -b, --bbox-overlap=OVERLAP - Set overlap when splitting polygons\n"
              << "  -i, --no-index             - Do not create spatial indexes in output db\n"
              << "  -d, --debug                - Enable debugging output\n"
              << "  -f, --overwrite            - Overwrite output file if it already exists\n"
              << "  -l, --output-lines         - Output coastlines as lines to database file\n"
              << "  -m, --max-points=NUM       - Split lines/polygons with more than this many\n"
              << "                               points (0 - disable splitting)\n"
              << "  -o, --output-database=FILE - Spatialite database file for output\n"
              << "  -p, --output-polygons=land|water|both|none\n"
              << "                             - Which polygons to write out (default: land)\n"
              << "  -r, --output-rings         - Output rings to database file\n"
              << "  -s, --srs=EPSGCODE         - Set SRS (4326 for WGS84 (default) or 3857)\n"
              << "  -S, --write-segments=FILE  - Write segments to given file\n"
              << "  -v, --verbose              - Verbose output\n"
              << "  -V, --version              - Show version and exit\n"
              << "\n";
}

