/*

  Copyright 2012-2018 Jochen Topf <jochen@topf.org>.

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

#include <osmium/osm/undirected_segment.hpp>
#include <osmium/util/memory_mapping.hpp>

#include <gdalcpp.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <vector>

#ifndef _MSC_VER
# include <unistd.h>
#else
# include <io.h>
#endif

using segvec = std::vector<osmium::UndirectedSegment>;

class InputFile {

    std::string m_filename;
    int m_fd;

public:

    explicit InputFile(const std::string& filename) :
        m_filename(filename),
        m_fd(::open(filename.c_str(), O_RDONLY)) {
        if (m_fd == -1) {
            throw std::system_error{errno, std::system_category(), std::string{"Opening '"} + filename + "' failed"};
        }
    }

    int fd() const noexcept {
        return m_fd;
    }

    std::size_t size() const {
        struct stat s; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
        if (::fstat(m_fd, &s) != 0) {
            throw std::system_error{errno, std::system_category(), std::string{"Can't get file size for '"} + m_filename + "'"};
        }
        return std::size_t(s.st_size);
    }

}; // class InputFile

void print_help() {
}

void add_segment(gdalcpp::Layer& layer, int change, const osmium::UndirectedSegment& segment) {
    auto linestring = std::unique_ptr<OGRLineString>{new OGRLineString()};
    linestring->addPoint(segment.first().lon(), segment.first().lat());
    linestring->addPoint(segment.second().lon(), segment.second().lat());

    gdalcpp::Feature feature(layer, std::move(linestring));
    feature.set_field("change", change);
    feature.add_to_layer();
}

void output_ogr(const std::string& filename, const std::string& driver_name, const segvec& removed_segments, const segvec& added_segments) {
    gdalcpp::Dataset dataset{driver_name, filename};

    gdalcpp::Layer layer{dataset, "changes", wkbLineString};
    layer.add_field("change", OFTInteger, 1);
    layer.start_transaction();

    for (const auto& segment : removed_segments) {
        add_segment(layer, 0, segment);
    }

    for (const auto& segment : added_segments) {
        add_segment(layer, 1, segment);
    }

    layer.commit_transaction();
}

int main(int argc, char *argv[]) {
    bool dump = false;
    std::string format = "ESRI Shapefile";
    std::string geom;

    static struct option long_options[] = {
        {"dump",         no_argument, nullptr, 'd'},
        {"format", required_argument, nullptr, 'f'},
        {"geom",   required_argument, nullptr, 'g'},
        {"help",         no_argument, nullptr, 'h'},
        {"version",      no_argument, nullptr, 'V'},
        {nullptr,                  0, nullptr, 0}
    };

    while (true) {
        const int c = getopt_long(argc, argv, "df:g:hV", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                dump = true;
                break;
            case 'f':
                format = optarg;
                break;
            case 'g':
                geom = optarg;
                break;
            case 'h': {
                std::cout << "Usage: " << argv[0] << " [OPTIONS] SEGFILE1 SEGFILE2\n";
                print_help();
                std::exit(return_code_ok);
            }
            case 'V':
                std::cout << "osmcoastline_segments version " OSMCOASTLINE_VERSION "\n"
                          << "Copyright (C) 2012-2018  Jochen Topf <jochen@topf.org>\n"
                          << "License: GNU GENERAL PUBLIC LICENSE Version 3 <https://gnu.org/licenses/gpl.html>.\n"
                          << "This is free software: you are free to change and redistribute it.\n"
                          << "There is NO WARRANTY, to the extent permitted by law.\n";
                std::exit(return_code_ok);
            default:
                break;
        }
    }

    if (optind != argc - 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] SEGFILE1 SEGFILE2\n";
        std::exit(return_code_cmdline);
    }

    segvec removed_segments;
    segvec added_segments;

    try {
        InputFile file1{argv[optind]};
        InputFile file2{argv[optind + 1]};

        osmium::util::TypedMemoryMapping<osmium::UndirectedSegment> m1{file1.size() / sizeof(osmium::UndirectedSegment), osmium::util::MemoryMapping::mapping_mode::readonly, file1.fd()};
        osmium::util::TypedMemoryMapping<osmium::UndirectedSegment> m2{file2.size() / sizeof(osmium::UndirectedSegment), osmium::util::MemoryMapping::mapping_mode::readonly, file2.fd()};

        std::set_difference(m1.cbegin(), m1.cend(), m2.cbegin(), m2.cend(), std::back_inserter(removed_segments));
        std::set_difference(m2.cbegin(), m2.cend(), m1.cbegin(), m1.cend(), std::back_inserter(added_segments));
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        std::exit(return_code_fatal);
    }

    if (dump) {
        std::cout << "Removed:\n";
        for (const auto& segment : removed_segments) {
            std::cout << "  " << segment << "\n";
        }

        std::cout << "Added:\n";
        for (const auto& segment : added_segments) {
            std::cout << "  " << segment << "\n";
        }
    } else if (!geom.empty()) {
        output_ogr(geom, format, removed_segments, added_segments);
    }

    return (removed_segments.empty() && added_segments.empty()) ? 0 : 1;
}

