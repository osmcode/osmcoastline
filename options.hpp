#ifndef OPTIONS_HPP
#define OPTIONS_HPP

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

/**
 * This class encapsulates the command line parsing.
 */
class Options {

public:

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

private:

    /**
     * Get EPSG code from text. This method knows about a few common cases
     * of specifying WGS84 or the "Google mercator" SRS. More are currently
     * not supported.
     */
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

#endif // OPTIONS_HPP
