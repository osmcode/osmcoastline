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

#include <osmium.hpp>
#include <osmium/storage/byid/sparsetable.hpp>
#include <osmium/handler/coordinates_for_ways.hpp>

#include "osmcoastline.hpp"

typedef Osmium::Storage::ById::SparseTable<Osmium::OSM::Position> storage_sparsetable_t;
typedef Osmium::Handler::CoordinatesForWays<storage_sparsetable_t, storage_sparsetable_t> cfw_handler_t;

class Segment {

public:

    Osmium::OSM::Position start;
    Osmium::OSM::Position end;

    Segment(const Osmium::OSM::Position& p1, const Osmium::OSM::Position& p2) :
        start(p1),
        end(p2) {
    }

    OGRLineString* ogr_linestring() const {
        OGRLineString* line = new OGRLineString;
        line->setNumPoints(2);
        line->setPoint(0, start.lon(), start.lat());
        line->setPoint(1, end.lon(), end.lat());
        line->setCoordinateDimension(2);

        return line;
    }

    bool outside_x_range(const Segment& other) const {
        if (start.x() > other.end.x()) {
            return true;
        }
        return false;
    }

    bool y_range_overlap(const Segment& other) const {
        int tmin = start.y() < end.y() ? start.y() : end.y();
        int tmax = start.y() < end.y() ? end.y() : start.y();
        int omin = other.start.y() < other.end.y() ? other.start.y() : other.end.y();
        int omax = other.start.y() < other.end.y() ? other.end.y() : other.start.y();
        if (tmin > omax || omin > tmax) {
            return false;
        }
        return true;
    }

    void check_intersection(const Segment& other, std::vector<Osmium::OSM::Position>& intersections) const {
        if (start == other.start || start == other.end || end == other.start || end == other.end) {
            return;
        }

        double denom = ((other.end.lat() - other.start.lat())*(end.lon() - start.lon())) -
                       ((other.end.lon() - other.start.lon())*(end.lat() - start.lat()));

        if (denom == 0) {
            return;
        }

        double nume_a = ((other.end.lon() - other.start.lon())*(start.lat() - other.start.lat())) -
                        ((other.end.lat() - other.start.lat())*(start.lon() - other.start.lon()));

        double nume_b = ((end.lon() - start.lon())*(start.lat() - other.start.lat())) -
                        ((end.lat() - start.lat())*(start.lon() - other.start.lon()));

        if ((denom > 0 && nume_a >= 0 && nume_a <= denom && nume_b >= 0 && nume_b <= denom) ||
            (denom < 0 && nume_a <= 0 && nume_a >= denom && nume_b <= 0 && nume_b >= denom)) {
            double ua = nume_a / denom;
            double ix = start.lon() + ua*(end.lon() - start.lon());
            double iy = start.lat() + ua*(end.lat() - start.lat());
            intersections.push_back(Osmium::OSM::Position(ix, iy));
        }
    }

    /// Segments are the same if their start and end points are the same.
    friend bool operator==(const Segment& s1, const Segment& s2) {
        return s1.start == s2.start && s1.end == s2.end;
    }

    /**
     * Segments are "smaller" if they are to the left and down of another
     * segment. Start position is checked first and only if they have the
     * same start position the end position is taken into account.
     */
    friend bool operator<(const Segment& s1, const Segment& s2) {
        if (s1.start.x() < s2.start.x()) {
            return true;
        }
        if (s1.start.x() > s2.start.x()) {
            return false;
        }
        if (s1.start.y() < s2.start.y()) {
            return true;
        }
        if (s1.start.y() > s2.start.y()) {
            return false;
        }

        if (s1.end.x() < s2.end.x()) {
            return true;
        }
        if (s1.end.x() > s2.end.x()) {
            return false;
        }
        if (s1.end.y() < s2.end.y()) {
            return true;
        }
        return false;
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
    std::vector<Segment>& m_segments;
    Osmium::OSM::WayNodeList& m_dpoints;

public:

    CoastlineWaysHandler2(cfw_handler_t& cfw, std::vector<Segment>& segments, Osmium::OSM::WayNodeList& dpoints) :
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
                    m_segments.push_back(Segment(p1, p2));
                } else {
                    m_segments.push_back(Segment(p2, p1));
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

    OGRLayer* layer_overlaps = data_source->CreateLayer("overlaps", &sparef, wkbLineString, NULL);
    if (layer_overlaps == NULL) {
        std::cerr << "Layer creation failed.\n";
        exit(return_code_fatal);
    }

    std::vector<Segment> segments;
    {
        storage_sparsetable_t store_pos;
        storage_sparsetable_t store_neg;
        cfw_handler_t handler_cfw(store_pos, store_neg);

        Osmium::OSMFile infile(argv[1]);
        CoastlineWaysHandler1 handler1(handler_cfw);

        std::cerr << "Reading nodes...\n";
        infile.read(handler1);

        segments.reserve(handler1.num_nodes() + 500);

        Osmium::OSM::WayNodeList dpoints;
        CoastlineWaysHandler2 handler2(handler_cfw, segments, dpoints);
        std::cerr << "Reading ways...\n";
        infile.read(handler2);

        for (Osmium::OSM::WayNodeList::const_iterator it = dpoints.begin(); it != dpoints.end(); ++it) {
            add_point(layer_points, it->lon(), it->lat(), it->ref(), "double_point");
        }
    }

    std::cerr << "Sorting...\n";
    std::sort(segments.begin(), segments.end());

    std::cerr << "Finding intersections...\n";
    std::vector<Osmium::OSM::Position> intersections;
    for (std::vector<Segment>::const_iterator it1 = segments.begin(); it1 != segments.end()-1; ++it1) {
        const Segment& s1 = *it1;
        for (std::vector<Segment>::const_iterator it2 = it1+1; it2 != segments.end(); ++it2) {
            const Segment& s2 = *it2;
            if (s1 == s2) {
                OGRFeature* feature = OGRFeature::CreateFeature(layer_overlaps->GetLayerDefn());
                OGRLineString* line = s1.ogr_linestring();
                feature->SetGeometryDirectly(line);

                if (layer_overlaps->CreateFeature(feature) != OGRERR_NONE) {
                    std::cerr << "Failed to create feature.\n";
                    exit(return_code_fatal);
                }

                OGRFeature::DestroyFeature(feature);
            } else {
                if (s2.outside_x_range(s1)) {
                    break;
                }
                if (s1.y_range_overlap(s2)) {
                    s1.check_intersection(s2, intersections);
                }
            }
        }
    }

    for (std::vector<Osmium::OSM::Position>::const_iterator it = intersections.begin(); it != intersections.end(); ++it) {
        add_point(layer_points, it->lon(), it->lat(), 0, "intersection");
    }

    OGRDataSource::DestroyDataSource(data_source);
}

