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

#include <algorithm>
#include <cassert>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <osmium/osm/undirected_segment.hpp>
#include <osmium/util/memory_mapping.hpp>

#include "ogr_include.hpp"
#include "osmcoastline.hpp"
#include "srs.hpp"

typedef std::vector<osmium::UndirectedSegment> segvec;

class InputFile {

    std::string m_filename;
    int m_fd;

public:

    InputFile(const std::string& filename) :
        m_filename(filename),
        m_fd(::open(filename.c_str(), O_RDONLY)) {
        if (m_fd == -1) {
            throw std::system_error(errno, std::system_category(), std::string("Opening '") + filename + "' failed");
        }
    }

    int fd() const {
        return m_fd;
    }

    size_t size() const {
        struct stat s;
        if (::fstat(m_fd, &s) != 0) {
            throw std::system_error(errno, std::system_category(), std::string("Can't get file size for '") + m_filename + "'");
        }
        return size_t(s.st_size);
    }

}; // class InputFile

void print_help() {
}

void add_segment(OGRLayer* layer, int change, const osmium::UndirectedSegment& segment) {
    OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());

    auto linestring = new OGRLineString();
    linestring->addPoint(segment.first().lon(), segment.first().lat());
    linestring->addPoint(segment.second().lon(), segment.second().lat());

    feature->SetGeometryDirectly(linestring);
    feature->SetField("change", change);

    if (layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature on layer 'changes'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

void output_ogr(const std::string& filename, const std::string& driver_name, const segvec& removed_segments, const segvec& added_segments) {
    OGRRegisterAll();

    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name.c_str());
    if (!driver) {
        std::cerr << driver_name << " driver not available.\n";
        exit(return_code_fatal);
    }

    //const char* options[] = { "SPATIALITE=yes", "OGR_SQLITE_SYNCHRONOUS=OFF", "INIT_WITH_EPSG=no", nullptr };
    const char* options[] = { nullptr };
    auto data_source = std::unique_ptr<OGRDataSource, OGRDataSourceDestroyer>(driver->CreateDataSource(filename.c_str(), const_cast<char**>(options)));
    if (!data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(return_code_fatal);
    }

    SRS srs;
    auto layer = data_source->CreateLayer("changes", srs.out(), wkbLineString, const_cast<char**>(options));
    if (!layer) {
        std::cerr << "Creating layer 'changes' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_change("change", OFTInteger);
    field_change.SetWidth(1);
    if (layer->CreateField(&field_change) != OGRERR_NONE ) {
        std::cerr << "Creating field 'change' on 'changes' layer failed.\n";
        exit(return_code_fatal);
    }

    layer->StartTransaction();

    for (const auto& segment : removed_segments) {
        add_segment(layer, 0, segment);
    }

    for (const auto& segment : added_segments) {
        add_segment(layer, 1, segment);
    }

    layer->CommitTransaction();
}

int main(int argc, char *argv[]) {
    bool dump = false;
    std::string format = "ESRI Shapefile";
    std::string geom;

    static struct option long_options[] = {
        {"dump",         no_argument, 0, 'd'},
        {"format", required_argument, 0, 'f'},
        {"geom",   required_argument, 0, 'g'},
        {"help",         no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "df:g:h", long_options, 0);
        if (c == -1)
            break;

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
                exit(return_code_ok);
            }
            default:
                break;
        }
    }

    if (optind != argc - 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] SEGFILE1 SEGFILE2\n";
        exit(return_code_cmdline);
    }

    segvec removed_segments;
    segvec added_segments;

    try {
        InputFile file1(argv[optind]);
        InputFile file2(argv[optind+1]);

        osmium::util::TypedMemoryMapping<osmium::UndirectedSegment> m1(file1.size() / sizeof(osmium::UndirectedSegment), false, file1.fd());
        osmium::util::TypedMemoryMapping<osmium::UndirectedSegment> m2(file2.size() / sizeof(osmium::UndirectedSegment), false, file2.fd());

        std::set_difference(m1.cbegin(), m1.cend(), m2.cbegin(), m2.cend(), std::back_inserter(removed_segments));
        std::set_difference(m2.cbegin(), m2.cend(), m1.cbegin(), m1.cend(), std::back_inserter(added_segments));
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        exit(return_code_fatal);
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
}

