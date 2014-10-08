/*

  Copyright 2012-2014 Jochen Topf <jochen@topf.org>.

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
#include <set>
#include <getopt.h>

#include <osmium/io/any_input.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/entity_bits.hpp>

#include "osmcoastline.hpp"

void print_help() {
    std::cout << "osmcoastline_filter [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help           - This help message\n"
              << "  -o, --output=OSMFILE - Where to write output (default: none)\n"
              << "\n";
}

int main(int argc, char* argv[]) {
    std::string output_filename;

    static struct option long_options[] = {
        {"help",         no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "ho:", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                print_help();
                exit(return_code_ok);
            case 'o':
                output_filename = optarg;
                break;
            default:
                exit(return_code_fatal);
        }
    }

    if (output_filename.empty()) {
        std::cerr << "Missing -o/--output=OSMFILE option\n";
        exit(return_code_cmdline);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: osmcoastline_filter [OPTIONS] OSMFILE\n";
        exit(return_code_cmdline);
    }

    osmium::io::Header header;
    header.set("generator", "osmcoastline_filter");

    osmium::io::File infile(argv[optind]);

    try {
        osmium::io::Writer writer(output_filename, header);

        std::set<osmium::object_id_type> ids;
        osmium::memory::Buffer output_buffer(10240);

        {
            osmium::io::Reader reader(infile, osmium::osm_entity_bits::way);
            while (auto input_buffer = reader.read()) {
                for (auto it = input_buffer.begin<const osmium::Way>(); it != input_buffer.end<const osmium::Way>(); ++it) {
                    const char* natural = it->get_value_by_key("natural");
                    if (natural && !strcmp(natural, "coastline")) {
                        output_buffer.add_item(*it);
                        output_buffer.commit();
                        if (output_buffer.committed() >= 10240) {
                            osmium::memory::Buffer new_buffer(10240);
                            std::swap(output_buffer, new_buffer);
                            writer(std::move(new_buffer));
                        }
                        for (const auto& nr : it->nodes()) {
                            ids.insert(nr.ref());
                        }
                    }
                }
            }
            reader.close();
        }

        {
            osmium::io::Reader reader(infile, osmium::osm_entity_bits::node);
            while (auto input_buffer = reader.read()) {
                for (auto it = input_buffer.begin<const osmium::Node>(); it != input_buffer.end<const osmium::Node>(); ++it) {
                    const char* natural = it->get_value_by_key("natural");
                    if ((ids.find(it->id()) != ids.end()) || (natural && !strcmp(natural, "coastline"))) {
                        output_buffer.add_item(*it);
                        output_buffer.commit();
                        if (output_buffer.committed() >= 10240) {
                            osmium::memory::Buffer new_buffer(10240);
                            std::swap(output_buffer, new_buffer);
                            writer(std::move(new_buffer));
                        }
                    }
                }
            }
            reader.close();
        }

        if (output_buffer.committed() > 0) {
            writer(std::move(output_buffer));
        }

        writer.close();
    } catch (osmium::io_error& e) {
        std::cerr << "io error: " << e.what() << "'\n";
        exit(return_code_fatal);
    }
}

