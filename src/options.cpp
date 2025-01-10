/*

  Copyright 2012-2025 Jochen Topf <jochen@topf.org>.

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

#include "return_codes.hpp"
#include "options.hpp"
#include "version.hpp"

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#include <osmium/io/pbf.hpp>

namespace {

void print_help() {
    std::cout << "Usage: osmcoastline [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help                 - This help message\n"
              << "  -c, --close-distance=DIST  - Distance between nodes under which open rings\n"
              << "                               are closed (0 - disable closing of rings)\n"
              << "  -b, --bbox-overlap=OVERLAP - Set overlap when splitting polygons\n"
              << "  -i, --no-index             - Do not create spatial indexes in output db\n"
              << "  -d, --debug                - Enable debugging output\n"
              << "  -e, --exit-ignore-warnings - Exit with code 0 even if there are warnings\n"
              << "  -f, --overwrite            - Overwrite output file if it already exists\n"
              << "  -g, --gdal-driver=DRIVER   - GDAL driver (SQLite or ESRI Shapefile)\n"
              << "  -l, --output-lines         - Output coastlines as lines to database file\n"
              << "  -m, --max-points=NUM       - Split lines/polygons with more than this many\n"
              << "                               points (0 - disable splitting)\n"
              << "  -o, --output-database=FILE - Database file for output\n"
              << "  -p, --output-polygons=land|water|both|none\n"
              << "                             - Which polygons to write out (default: land)\n"
              << "  -r, --output-rings         - Output rings to database file\n"
              << "  -s, --srs=EPSGCODE         - Set SRS (4326 for WGS84 (default) or 3857)\n"
              << "  -S, --write-segments=FILE  - Write segments to given file\n"
              << "  -v, --verbose              - Verbose output\n"
              << "  -V, --version              - Show version and exit\n"
              << "\n";
}

void print_version() {
    std::cout << "osmcoastline " << get_osmcoastline_long_version() << "\n"
              << get_libosmium_version() << '\n'
              << "Supported PBF compression types:";
    for (const auto& type : osmium::io::supported_pbf_compression_types()) {
        std::cout << " " << type;
    }
    std::cout << "\n\nCopyright (C) 2012-2025  Jochen Topf <jochen@topf.org>\n"
              << "License: GNU GENERAL PUBLIC LICENSE Version 3 <https://gnu.org/licenses/gpl.html>.\n"
              << "This is free software: you are free to change and redistribute it.\n"
              << "There is NO WARRANTY, to the extent permitted by law.\n";
}

/**
 * Get EPSG code from text. This method knows about a few common cases
 * of specifying WGS84 or the "Web Mercator" SRS. More are currently
 * not supported.
 */
int get_epsg(const char* text) {
    if (!strcasecmp(text, "WGS84") || !std::strcmp(text, "4326")) {
        return 4326;
    }
    if (!std::strcmp(text, "3857")) {
        return 3857;
    }
    if (!std::strcmp(text, "3785") || !std::strcmp(text, "900913")) {
        std::cerr << "Please use code 3857 for the 'Web Mercator' projection!\n";
        std::exit(return_code_cmdline);
    }
    std::cerr << "Unknown SRS '" << text << "'. Currently only 4326 (WGS84) and 3857 ('Web Mercator') are supported.\n";
    std::exit(return_code_cmdline);
}

} // anonymous namespace

int Options::parse(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"bbox-overlap",    required_argument, nullptr, 'b'},
        {"close-distance",  required_argument, nullptr, 'c'},
        {"no-index",              no_argument, nullptr, 'i'},
        {"debug",                 no_argument, nullptr, 'd'},
        {"exit-ignore-warnings",  no_argument, nullptr, 'e'},
        {"gdal-driver",     required_argument, nullptr, 'g'},
        {"help",                  no_argument, nullptr, 'h'},
        {"output-lines",          no_argument, nullptr, 'l'},
        {"max-points",      required_argument, nullptr, 'm'},
        {"output-database", required_argument, nullptr, 'o'},
        {"output-polygons", required_argument, nullptr, 'p'},
        {"output-rings",          no_argument, nullptr, 'r'},
        {"overwrite",             no_argument, nullptr, 'f'},
        {"srs",             required_argument, nullptr, 's'},
        {"write-segments",  required_argument, nullptr, 'S'},
        {"verbose",               no_argument, nullptr, 'v'},
        {"version",               no_argument, nullptr, 'V'},
        {nullptr,                           0, nullptr, 0}
    };

    while (true) {
        const int c = getopt_long(argc, argv, "b:c:ideg:hlm:o:p:rfs:S:vV", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'b':
                bbox_overlap = std::atof(optarg); // NOLINT(cert-err34-c) atof is good enough for this use case
                break;
            case 'c':
                close_distance = std::atoi(optarg); // NOLINT(cert-err34-c) atoi is good enough for this use case
                break;
            case 'i':
                create_index = false;
                break;
            case 'd':
                debug = true;
                std::cerr << "Enabled debug option\n";
                break;
            case 'e':
                exit_ignore_warnings = true;
                break;
            case 'h':
                print_help();
                return return_code_ok;
            case 'g':
                driver = optarg;
                break;
            case 'l':
                output_lines = true;
                break;
            case 'm':
                max_points_in_polygon = std::atoi(optarg); // NOLINT(cert-err34-c) atoi is good enough for this use case
                if (max_points_in_polygon == 0) {
                    split_large_polygons = false;
                }
                break;
            case 'p':
                if (!std::strcmp(optarg, "none")) {
                    output_polygons = output_polygon_type::none;
                } else if (!std::strcmp(optarg, "land")) {
                    output_polygons = output_polygon_type::land;
                } else if (!std::strcmp(optarg, "water")) {
                    output_polygons = output_polygon_type::water;
                } else if (!std::strcmp(optarg, "both")) {
                    output_polygons = output_polygon_type::both;
                } else {
                    std::cerr << "Unknown argument '" << optarg << "' for -p/--output-polygon option\n";
                    return return_code_cmdline;
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
                print_version();
                return return_code_ok;
            default:
                return return_code_cmdline;
        }
    }

    if (!split_large_polygons && (output_polygons == output_polygon_type::water || output_polygons == output_polygon_type::both)) {
        std::cerr << "Can not use -m/--max-points=0 when writing out water polygons\n";
        return return_code_cmdline;
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: osmcoastline [OPTIONS] OSMFILE\n";
        return return_code_cmdline;
    }

    if (output_database.empty()) {
        std::cerr << "Missing --output-database/-o option.\n";
        return return_code_cmdline;
    }

    if (bbox_overlap == -1) {
        if (epsg == 4326) {
            bbox_overlap = 0.0001;
        } else {
            bbox_overlap = 10;
        }
    }

    inputfile = argv[optind];

    return -1;
}

