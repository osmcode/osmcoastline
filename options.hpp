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
    std::string inputfile;

    /// Overlap when splitting polygons.
    double bbox_overlap;

    /**
     * If the distance between two ring-endnodes is smaller than this the ring
     * can be closed there.
     */
    double close_distance;

    /// Attempt to close unclosed rings?
    bool close_rings;

    /// Add spatial index to Spatialite database tables?
    bool create_index;

    /// Show debug output?
    bool debug;

    /// Maximum number of points in polygons.
    int max_points_in_polygon;

    /// Should large polygons be split?
    bool split_large_polygons;

    /// Should the polygons output table be populated?
    bool output_polygons;

    /// Output Spatialite database file name.
    std::string output_database;

    /// Output OSM "raw data" file name.
    std::string output_osm;

    /// Should the rings output table be populated?
    bool output_rings;

    /// EPSG code of output SRS.
    int epsg;

    /// Verbose output?
    bool verbose;

    /// Output water polygons instead of land polygons?
    bool water;

    Options(int argc, char* argv[]) :
        inputfile(),
        bbox_overlap(-1),
        close_distance(1.0),
        close_rings(true),
        create_index(false),
        debug(false),
        max_points_in_polygon(1000),
        split_large_polygons(true),
        output_polygons(true),
        output_database(),
        output_osm(),
        output_rings(false),
        epsg(4326),
        verbose(false),
        water(false)
    {
        static struct option long_options[] = {
            {"bbox-overlap",    required_argument, 0, 'b'},
            {"close-distance",  required_argument, 0, 'c'},
            {"create-index",          no_argument, 0, 'i'},
            {"debug",                 no_argument, 0, 'd'},
            {"help",                  no_argument, 0, 'h'},
            {"max-points",      required_argument, 0, 'm'},
            {"no-polygons",           no_argument, 0, 'p'},
            {"output-database", required_argument, 0, 'o'},
            {"output-osm",      required_argument, 0, 'O'},
            {"output-rings",          no_argument, 0, 'r'},
            {"srs",             required_argument, 0, 's'},
            {"verbose",               no_argument, 0, 'v'},
            {"water",                 no_argument, 0, 'w'},
            {0, 0, 0, 0}
        };

        while (1) {
            int c = getopt_long(argc, argv, "b:c:idhm:po:O:rs:vw", long_options, 0);
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
                    create_index = true;
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
                case 's':
                    epsg = get_epsg(optarg);
                    break;
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
                  << "  -h, --help                 - This help message\n"
                  << "  -c, --close-distance       - Distance between nodes under which open rings\n"
                  << "                               are closed (0 - disable closing of rings)\n"
                  << "  -b, --bbox-overlap         - Set overlap when splitting polygons\n"
                  << "  -i, --create-index         - Create spatial indexes in output database\n"
                  << "  -d, --debug                - Enable debugging output\n"
                  << "  -m, --max-points           - Split polygons with more than this many points\n"
                  << "                               (0 - disable splitting)\n"
                  << "  -p, --no-polygons          - Do not create polygons\n"
                  << "  -o, --output-database=FILE - Spatialite database file for output\n"
                  << "  -O, --output-osm=FILE      - Write raw OSM output to file\n"
                  << "  -r, --output-rings         - Output rings to database file\n"
                  << "  -s, --srs=EPSGCODE         - Set SRS (4326 for WGS84 (default) or 3857)\n"
                  << "  -v, --verbose              - Verbose output\n"
                  << "\n"
                  << "At least one of --output-database/-o and --output-osm/-O is needed.\n"
        ;
    }

};

#endif // OPTIONS_HPP
