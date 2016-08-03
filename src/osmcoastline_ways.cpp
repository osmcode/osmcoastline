/*

  Copyright 2012-2016 Jochen Topf <jochen@topf.org>.

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

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include <osmium/geom/haversine.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#include <gdalcpp.hpp>

#include "return_codes.hpp"

typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_type;
typedef osmium::handler::NodeLocationsForWays<index_type, index_type> node_location_handler_type;

class CoastlineWaysHandler : public osmium::handler::Handler {

    double m_length;

    gdalcpp::Dataset m_dataset;
    gdalcpp::Layer m_layer_ways;

    osmium::geom::OGRFactory<> m_factory;

public:

    explicit CoastlineWaysHandler(const std::string& db_filename) :
        m_length(0.0),
        m_dataset("SQLite", db_filename, gdalcpp::SRS{}, {"SPATIALITE=TRUE", "INIT_WITH_EPSG=no" }),
        m_layer_ways(m_dataset, "ways", wkbLineString) {

        m_layer_ways.add_field("way_id", OFTString, 10);
        m_layer_ways.add_field("name",   OFTString, 100);
        m_layer_ways.add_field("source", OFTString, 255);
        m_layer_ways.add_field("bogus",  OFTString, 1);
        m_layer_ways.start_transaction();
    }

    ~CoastlineWaysHandler() {
        m_layer_ways.commit_transaction();
    }

    void way(osmium::Way& way) {
        m_length += osmium::geom::haversine::distance(way.nodes());
        try {
            std::unique_ptr<OGRLineString> ogrlinestring = m_factory.create_linestring(way);
            gdalcpp::Feature feature(m_layer_ways, std::move(ogrlinestring));
            feature.set_field("way_id", std::to_string(way.id()).c_str());
            feature.set_field("name", way.tags().get_value_by_key("name"));
            feature.set_field("source", way.tags().get_value_by_key("source"));

            const char* coastline = way.tags().get_value_by_key("coastline");
            feature.set_field("bogus", (coastline && !std::strcmp(coastline, "bogus")) ? "t" : "f");
            feature.add_to_layer();
        } catch (osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

    double sum_length() const {
        return m_length;
    }

};

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        if (!std::strcmp(argv[1], "--help") || !std::strcmp(argv[1], "-h")) {
            std::cout << "Usage: osmcoastline_ways OSMFILE [WAYSDB]\n";
            exit(return_code_ok);
        }

        if (!std::strcmp(argv[1], "--version") || !std::strcmp(argv[1], "-V")) {
            std::cout << "osmcoastline_ways version " OSMCOASTLINE_VERSION "\n"
                      << "Copyright (C) 2012-2016  Jochen Topf <jochen@topf.org>\n"
                      << "License: GNU GENERAL PUBLIC LICENSE Version 3 <http://gnu.org/licenses/gpl.html>.\n"
                      << "This is free software: you are free to change and redistribute it.\n"
                      << "There is NO WARRANTY, to the extent permitted by law.\n";
            exit(return_code_ok);
        }
    }

    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: osmcoastline_ways OSMFILE [WAYSDB]\n";
        exit(return_code_cmdline);
    }

    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "OFF");

    std::string input_osm_filename { argv[1] };
    std::string output_db_filename { "coastline-ways.db" };

    if (argc >= 3) {
        output_db_filename = argv[2];
    }

    index_type store_pos;
    index_type store_neg;
    node_location_handler_type location_handler(store_pos, store_neg);

    osmium::io::File infile(input_osm_filename);
    osmium::io::Reader reader1(infile, osmium::osm_entity_bits::node);
    osmium::apply(reader1, location_handler);
    reader1.close();

    CoastlineWaysHandler coastline_ways_handler(output_db_filename);
    osmium::io::Reader reader2(infile, osmium::osm_entity_bits::way);
    osmium::apply(reader2, location_handler, coastline_ways_handler);
    reader2.close();

    std::cerr << "Sum of way lengths: " << std::fixed << (coastline_ways_handler.sum_length() / 1000) << "km\n";
}

