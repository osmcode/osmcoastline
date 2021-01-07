/*

  Copyright 2012-2021 Jochen Topf <jochen@topf.org>.

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
#include "version.hpp"

#include <osmium/index/id_set.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/error.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/header.hpp>
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

void print_help() {
    std::cout << "Usage: osmcoastline_filter [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help           - This help message\n"
              << "  -o, --output=OSMFILE - Where to write output (default: none)\n"
              << "  -v, --verbose        - Verbose output\n"
              << "  -V, --version        - Show version and exit\n"
              << "\n";
}

int main(int argc, char* argv[]) {
    std::string output_filename;
    bool verbose = false;

    static struct option long_options[] = {
        {"help",         no_argument, nullptr, 'h'},
        {"output", required_argument, nullptr, 'o'},
        {"verbose",      no_argument, nullptr, 'v'},
        {"version",      no_argument, nullptr, 'V'},
        {nullptr,                  0, nullptr, 0}
    };

    while (true) {
        const int c = getopt_long(argc, argv, "ho:vV", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help();
                std::exit(return_code_ok);
            case 'o':
                output_filename = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'V':
                std::cout << "osmcoastline_filter " << get_osmcoastline_long_version() << " / " << get_libosmium_version() << '\n'
                          << "Copyright (C) 2012-2021  Jochen Topf <jochen@topf.org>\n"
                          << "License: GNU GENERAL PUBLIC LICENSE Version 3 <https://gnu.org/licenses/gpl.html>.\n"
                          << "This is free software: you are free to change and redistribute it.\n"
                          << "There is NO WARRANTY, to the extent permitted by law.\n";
                std::exit(return_code_ok);
            default:
                std::exit(return_code_fatal);
        }
    }

    if (output_filename.empty()) {
        std::cerr << "Missing -o/--output=OSMFILE option\n";
        std::exit(return_code_cmdline);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: osmcoastline_filter [OPTIONS] OSMFILE\n";
        std::exit(return_code_cmdline);
    }

    try {
        // The vout object is an output stream we can write to instead of
        // std::cerr. Nothing is written if we are not in verbose mode.
        // The running time will be prepended to output lines.
        osmium::util::VerboseOutput vout{verbose};

        osmium::io::Header header;
        header.set("generator", std::string{"osmcoastline_filter/"} + get_osmcoastline_version());
        header.add_box(osmium::Box{-180.0, -90.0, 180.0, 90.0});

        osmium::io::File infile{argv[optind]};

        vout << "Started osmcoastline_filter " << get_osmcoastline_long_version() << " / " << get_libosmium_version() << '\n';

        osmium::io::Writer writer{output_filename, header};
        auto output_it = osmium::io::make_output_iterator(writer);

        osmium::index::IdSetSmall<osmium::object_id_type> ids;

        vout << "Reading ways (1st pass through input file)...\n";
        {
            osmium::io::Reader reader{infile, osmium::osm_entity_bits::way};
            const auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);
            for (const osmium::Way& way : ways) {
                if (way.tags().has_tag("natural", "coastline")) {
                    *output_it++ = way;
                    for (const auto& nr : way.nodes()) {
                        ids.set(nr.ref());
                    }
                }
            }
            reader.close();
        }

        vout << "Preparing node ID list...\n";
        ids.sort_unique();

        vout << "Reading nodes (2nd pass through input file)...\n";
        {
            osmium::io::Reader reader{infile, osmium::osm_entity_bits::node};
            const auto nodes = osmium::io::make_input_iterator_range<const osmium::Node>(reader);

            auto first = ids.cbegin();
            const auto last = ids.cend();
            std::copy_if(nodes.cbegin(), nodes.cend(), output_it, [&first, &last](const osmium::Node& node){
                while (*first < node.id() && first != last) {
                    ++first;
                }

                if (node.id() == *first) {
                    if (first != last) {
                        ++first;
                    }
                    return true;
                }

                return node.tags().has_tag("natural", "coastline");
            });

            reader.close();
        }

        writer.close();

        vout << "All done.\n";
        osmium::MemoryUsage mem;
        if (mem.current() > 0) {
            vout << "Memory used: current: " << mem.current() << " MBytes\n"
                << "             peak:    " << mem.peak() << " MBytes\n";
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::exit(return_code_fatal);
    }
}

