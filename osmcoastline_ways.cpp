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

#include <iostream>
#include <set>

#include <ogrsf_frmts.h>

#include <osmium.hpp>
#include <osmium/storage/byid/sparsetable.hpp>
#include <osmium/handler/coordinates_for_ways.hpp>
#include <osmium/geometry/haversine.hpp>

#include "osmcoastline.hpp"

typedef Osmium::Storage::ById::SparseTable<Osmium::OSM::Position> storage_sparsetable_t;
typedef Osmium::Handler::CoordinatesForWays<storage_sparsetable_t, storage_sparsetable_t> cfw_handler_t;

class CoastlineWaysHandler1 : public Osmium::Handler::Base {

    cfw_handler_t& m_cfw;

public:
    
    CoastlineWaysHandler1(cfw_handler_t& cfw) :
        m_cfw(cfw) {
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        m_cfw.node(node);
    }

    void after_nodes() const {
        throw Osmium::Input::StopReading();
    }

};

class CoastlineWaysHandler2 : public Osmium::Handler::Base {

    cfw_handler_t& m_cfw;
    double m_length;

    OGRDataSource* m_data_source;
    OGRLayer* m_layer_ways;

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

        m_layer_ways->StartTransaction();
    }

    ~CoastlineWaysHandler2() {
        m_layer_ways->CommitTransaction();
        OGRDataSource::DestroyDataSource(m_data_source);
    }

    void way(const shared_ptr<Osmium::OSM::Way>& way) {
        m_cfw.way(way);
        m_length += Osmium::Geometry::Haversine::distance(way->nodes());
        try {
            Osmium::Geometry::LineString linestring(*way);

            OGRFeature* feature = OGRFeature::CreateFeature(m_layer_ways->GetLayerDefn());
            OGRLineString* ogrlinestring = linestring.create_ogr_geometry();
            feature->SetGeometry(ogrlinestring);
            feature->SetField("way_id", way->id());
            feature->SetField("name", way->tags().get_tag_by_key("name"));
            feature->SetField("source", way->tags().get_tag_by_key("source"));

            if (m_layer_ways->CreateFeature(feature) != OGRERR_NONE) {
                std::cerr << "Failed to create feature.\n";
                exit(1);
            }

            OGRFeature::DestroyFeature(feature);
            delete ogrlinestring;
        } catch (Osmium::Exception::IllegalGeometry) {
            std::cerr << "Ignoring illegal geometry for way " << way->id() << ".\n";
        }
    }

    void after_ways() const {
        throw Osmium::Input::StopReading();
    }

    double sum_length() const {
        return m_length;
    }

};

int main(int argc, char* argv[]) {
    Osmium::init(false);

    if (argc != 2) {
        std::cerr << "Usage: osmcoastline_ways OSMFILE\n";
        exit(return_code_cmdline);
    }

    storage_sparsetable_t store_pos;
    storage_sparsetable_t store_neg;
    cfw_handler_t handler_cfw(store_pos, store_neg);

    Osmium::OSMFile infile(argv[1]);
    CoastlineWaysHandler1 handler1(handler_cfw);
    infile.read(handler1);
    CoastlineWaysHandler2 handler2(handler_cfw);
    infile.read(handler2);

    std::cerr << "Sum of way lengths: " << handler2.sum_length() << "m\n";
}

