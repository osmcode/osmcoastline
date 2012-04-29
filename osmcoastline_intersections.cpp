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
#include <vector>

#include <ogrsf_frmts.h>

#include <CGAL/Cartesian.h>
#include <CGAL/MP_Float.h>
#include <CGAL/Quotient.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Sweep_line_2_algorithms.h>

typedef CGAL::Quotient<CGAL::MP_Float>     NT;
typedef CGAL::Cartesian<NT>                Kernel;
typedef Kernel::Point_2                    Point_2;
typedef CGAL::Arr_segment_traits_2<Kernel> Traits_2;
typedef Traits_2::Curve_2                  Segment_2;
typedef std::vector<Segment_2>             Segments;

#include <osmium.hpp>
#include <osmium/storage/byid/sparsetable.hpp>
#include <osmium/handler/coordinates_for_ways.hpp>

#include "osmcoastline.hpp"

typedef Osmium::Storage::ById::SparseTable<Osmium::OSM::Position> storage_sparsetable_t;
typedef Osmium::Handler::CoordinatesForWays<storage_sparsetable_t, storage_sparsetable_t> cfw_handler_t;

class OSMSegment {

public:

    Osmium::OSM::Position start;
    Osmium::OSM::Position end;

    OSMSegment(const Osmium::OSM::Position& p1, const Osmium::OSM::Position& p2) :
        start(p1),
        end(p2) {
    }

};

class CoastlineWaysHandler1 : public Osmium::Handler::Base {

    cfw_handler_t& m_cfw;
    size_t m_nodes;

public:
    
    CoastlineWaysHandler1(cfw_handler_t& cfw) :
        m_cfw(cfw),
        m_nodes(0) {
    }

    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        m_nodes++;
        m_cfw.node(node);
    }

    void after_nodes() const {
        throw Osmium::Input::StopReading();
    }

    size_t num_nodes() const {
        return m_nodes;
    }

};

class CoastlineWaysHandler2 : public Osmium::Handler::Base {

    cfw_handler_t& m_cfw;
    std::vector<OSMSegment>& m_segments;
    Osmium::OSM::WayNodeList& m_dpoints;

public:

    CoastlineWaysHandler2(cfw_handler_t& cfw, std::vector<OSMSegment>& segments, Osmium::OSM::WayNodeList& dpoints) :
        m_cfw(cfw),
        m_segments(segments),
        m_dpoints(dpoints) {
    }

    void way(const shared_ptr<Osmium::OSM::Way const>& way) {
        if (way->nodes().size() <= 1) {
            return;
        }

        for (Osmium::OSM::WayNodeList::const_iterator it = way->nodes().begin(); it != way->nodes().end()-1; ++it) {
            const Osmium::OSM::Position& p1 = m_cfw.get_node_pos(it->ref());
            const Osmium::OSM::Position& p2 = m_cfw.get_node_pos((it+1)->ref());
            if (p1 != p2) {
                if (p1.x() <= p2.x()) {
                    m_segments.push_back(OSMSegment(p1, p2));
                } else {
                    m_segments.push_back(OSMSegment(p2, p1));
                }
            } else {
                m_dpoints.push_back(Osmium::OSM::WayNode(it->ref(), p1));
            }
        }
    }

    void after_ways() const {
        throw Osmium::Input::StopReading();
    }

};

void add_point(OGRLayer* layer, double lon, double lat, osm_object_id_t osm_id, const char* error) {
    OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    OGRPoint point(lon, lat);
    feature->SetGeometry(&point);
    feature->SetField("osm_id", osm_id);
    feature->SetField("error", error);

    if (layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

int main(int argc, char* argv[]) {
    Osmium::init(false);

    if (argc != 2) {
        std::cerr << "Usage: osmcoastline_intersections OSMFILE\n";
        exit(return_code_cmdline);
    }

    OGRRegisterAll();

    const char* driver_name = "SQLite";
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name);
    if (driver == NULL) {
        std::cerr << driver_name << " driver not available.\n";
        exit(return_code_fatal);
    }

    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
    const char* options[] = { "SPATIALITE=TRUE", NULL };
    OGRDataSource* data_source = driver->CreateDataSource("coastline-intersections.db", const_cast<char**>(options));
    if (data_source == NULL) {
        std::cerr << "Creation of output file failed.\n";
        exit(return_code_fatal);
    }

    OGRSpatialReference sparef;
    sparef.SetWellKnownGeogCS("WGS84");

    OGRLayer* layer_points = data_source->CreateLayer("intersections", &sparef, wkbPoint, NULL);
    if (layer_points == NULL) {
        std::cerr << "Layer creation failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_osm_id("osm_id", OFTString);
    field_osm_id.SetWidth(10);
    if (layer_points->CreateField(&field_osm_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'osm_id' on 'points' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_error("error", OFTString);
    field_error.SetWidth(16);
    if (layer_points->CreateField(&field_error) != OGRERR_NONE ) {
        std::cerr << "Creating field 'error' on 'points' layer failed.\n";
        exit(return_code_fatal);
    }

    layer_points->StartTransaction();

    std::vector<OSMSegment> osmsegments;
    {
        storage_sparsetable_t store_pos;
        storage_sparsetable_t store_neg;
        cfw_handler_t handler_cfw(store_pos, store_neg);

        Osmium::OSMFile infile(argv[1]);
        CoastlineWaysHandler1 handler1(handler_cfw);

        std::cerr << "Reading nodes...\n";
        infile.read(handler1);

        osmsegments.reserve(handler1.num_nodes() + 500);

        Osmium::OSM::WayNodeList dpoints;
        CoastlineWaysHandler2 handler2(handler_cfw, osmsegments, dpoints);
        std::cerr << "Reading ways...\n";
        infile.read(handler2);

        for (Osmium::OSM::WayNodeList::const_iterator it = dpoints.begin(); it != dpoints.end(); ++it) {
            add_point(layer_points, it->lon(), it->lat(), it->ref(), "double_point");
        }
    }

    const int width = 10;
    const int overlap = 1;
    for (int minx=-180, maxx=minx+width+overlap; minx <= 180; minx += width, maxx += width) {
        std::cerr << "Interval [" << minx << ", " << maxx << "]\n";

        Segments segments;
        for (std::vector<OSMSegment>::const_iterator it = osmsegments.begin(); it != osmsegments.end(); ++it) {
            if (it->start.lon() >= minx && it->start.lon() <= maxx) {
                segments.push_back( Segment_2(Point_2(it->start.lon(), it->start.lat()), Point_2(it->end.lon(), it->end.lat())) );
            }
        }
        
        std::cerr << "Computing intersections...\n";
        std::vector<Point_2> points;
        CGAL::compute_intersection_points(segments.begin(), segments.end(), std::back_inserter(points));

        std::cerr << "Writing intersections...\n";
        for (std::vector<Point_2>::const_iterator it = points.begin(); it != points.end(); ++it) {
            std::cout << "(" << CGAL::to_double(it->x()) << ", " << CGAL::to_double(it->y()) << ")\n";
            add_point(layer_points, CGAL::to_double(it->x()), CGAL::to_double(it->y()), 0, "intersection");
        }
    }

    layer_points->CommitTransaction();

    OGRDataSource::DestroyDataSource(data_source);
}

