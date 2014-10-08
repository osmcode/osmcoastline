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

#include <ogrsf_frmts.h>

#include <boost/lexical_cast.hpp>

#include <osmium/geom/haversine.hpp>
#include <osmium/geom/ogr.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_table.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#include "osmcoastline.hpp"

typedef osmium::index::map::SparseTable<osmium::unsigned_object_id_type, osmium::Location> storage_sparsetable_t;
typedef osmium::handler::NodeLocationsForWays<storage_sparsetable_t, storage_sparsetable_t> cfw_handler_t;

class CoastlineWaysHandler1 : public osmium::handler::Handler {

    cfw_handler_t& m_cfw;

public:
    
    CoastlineWaysHandler1(cfw_handler_t& cfw) :
        m_cfw(cfw) {
    }

    void node(const osmium::Node& node) {
        m_cfw.node(node);
    }

};

class CoastlineWaysHandler2 : public osmium::handler::Handler {

    cfw_handler_t& m_cfw;
    double m_length;

    OGRDataSource* m_data_source;
    OGRLayer* m_layer_ways;

    osmium::geom::OGRFactory<> m_factory;

public:

    CoastlineWaysHandler2(cfw_handler_t& cfw) :
        m_cfw(cfw),
        m_length(0.0) {
        OGRRegisterAll();

        const char* driver_name = "SQLite";
        OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name);
        if (driver == NULL) {
            std::cerr << driver_name << " driver not available.\n";
            exit(return_code_fatal);
        }

        CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
        const char* options[] = { "SPATIALITE=TRUE", NULL };
        m_data_source = driver->CreateDataSource("coastline-ways.db", const_cast<char**>(options));
        if (m_data_source == NULL) {
            std::cerr << "Creation of output file failed.\n";
            exit(return_code_fatal);
        }

        OGRSpatialReference sparef;
        sparef.SetWellKnownGeogCS("WGS84");
        m_layer_ways = m_data_source->CreateLayer("ways", &sparef, wkbLineString, NULL);
        if (m_layer_ways == NULL) {
            std::cerr << "Layer creation failed.\n";
            exit(return_code_fatal);
        }

        OGRFieldDefn field_way_id("way_id", OFTString);
        field_way_id.SetWidth(10);
        if (m_layer_ways->CreateField(&field_way_id) != OGRERR_NONE ) {
            std::cerr << "Creating field 'way_id' on 'ways' layer failed.\n";
            exit(return_code_fatal);
        }

        OGRFieldDefn field_name("name", OFTString);
        field_name.SetWidth(100);
        if (m_layer_ways->CreateField(&field_name) != OGRERR_NONE ) {
            std::cerr << "Creating field 'name' on 'ways' layer failed.\n";
            exit(return_code_fatal);
        }

        OGRFieldDefn field_source("source", OFTString);
        field_source.SetWidth(255);
        if (m_layer_ways->CreateField(&field_source) != OGRERR_NONE ) {
            std::cerr << "Creating field 'source' on 'ways' layer failed.\n";
            exit(return_code_fatal);
        }

        OGRFieldDefn field_bogus("bogus", OFTString);
        field_bogus.SetWidth(1);
        if (m_layer_ways->CreateField(&field_bogus) != OGRERR_NONE ) {
            std::cerr << "Creating field 'bogus' on 'ways' layer failed.\n";
            exit(return_code_fatal);
        }

        m_layer_ways->StartTransaction();
    }

    ~CoastlineWaysHandler2() {
        m_layer_ways->CommitTransaction();
        OGRDataSource::DestroyDataSource(m_data_source);
    }

    void way(osmium::Way& way) {
        m_cfw.way(way);
        m_length += osmium::geom::haversine::distance(way.nodes());
        try {
            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_ways->GetLayerDefn());
            OGRLineString* ogrlinestring = m_factory.create_linestring(way).release();
            feature->SetGeometry(ogrlinestring);
            feature->SetField("way_id", boost::lexical_cast<std::string>(way.id()).c_str());
            feature->SetField("name", way.tags().get_value_by_key("name"));
            feature->SetField("source", way.tags().get_value_by_key("source"));

            const char* coastline = way.tags().get_value_by_key("coastline");
            feature->SetField("bogus", (coastline && !strcmp(coastline, "bogus")) ? "t" : "f");

            if (m_layer_ways->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
            delete ogrlinestring;
        } catch (osmium::geometry_error&) {
            std::cerr << "Ignoring illegal geometry for way " << way.id() << ".\n";
        }
    }

    double sum_length() const {
        return m_length;
    }

};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: osmcoastline_ways OSMFILE\n";
        exit(return_code_cmdline);
    }

    storage_sparsetable_t store_pos;
    storage_sparsetable_t store_neg;
    cfw_handler_t handler_cfw(store_pos, store_neg);

    osmium::io::File infile(argv[1]);
    CoastlineWaysHandler1 handler1(handler_cfw);
    osmium::io::Reader reader1(infile, osmium::osm_entity_bits::node);
    osmium::apply(reader1, handler1);
    CoastlineWaysHandler2 handler2(handler_cfw);
    osmium::io::Reader reader2(infile, osmium::osm_entity_bits::way);
    osmium::apply(reader2, handler2);

    std::cerr << "Sum of way lengths: " << handler2.sum_length() << "m\n";
}

