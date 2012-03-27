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
#include <sstream>
#include <string>

#include <ogrsf_frmts.h>
#include <geos_c.h>

#include "osmcoastline.hpp"
#include "output_layers.hpp"

/***************************************************************/

LayerErrorPoints::LayerErrorPoints(OGRDataSource* data_source, OGRSpatialReference* srs, const char** options) {
    m_layer = data_source->CreateLayer("error_points", srs, wkbPoint, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'error_points' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_id("id", OFTInteger);
    field_id.SetWidth(10);
    if (m_layer->CreateField(&field_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'id' on 'error_points' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_error("error", OFTString);
    field_error.SetWidth(16);
    if (m_layer->CreateField(&field_error) != OGRERR_NONE ) {
        std::cerr << "Creating field 'error' on 'error_points' layer failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

LayerErrorPoints::~LayerErrorPoints() {
    m_layer->CommitTransaction();
}

OGRErr LayerErrorPoints::commit() {
    return m_layer->CommitTransaction();
}

void LayerErrorPoints::add(OGRPoint* point, int id, const char* error) {
    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(point);
    feature->SetField("id", id);
    feature->SetField("error", error);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature on layer 'error_points'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerErrorLines::LayerErrorLines(OGRDataSource* data_source, OGRSpatialReference* srs, const char** options) {
    m_layer = data_source->CreateLayer("error_lines", srs, wkbLineString, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'error_lines' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_id("id", OFTInteger);
    field_id.SetWidth(10);
    if (m_layer->CreateField(&field_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'id' on 'error_lines' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_simple("simple", OFTInteger);
    field_simple.SetWidth(1);
    if (m_layer->CreateField(&field_simple) != OGRERR_NONE ) {
        std::cerr << "Creating field 'simple' on 'error_lines' layer failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

LayerErrorLines::~LayerErrorLines() {
    m_layer->CommitTransaction();
}

OGRErr LayerErrorLines::commit() {
    return m_layer->CommitTransaction();
}

void LayerErrorLines::add(OGRLineString* linestring, int id, bool is_simple) {
    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(linestring);
    feature->SetField("id", id);
    feature->SetField("simple", is_simple ? 1 : 0);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature on layer 'error_lines'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerRings::LayerRings(OGRDataSource* data_source, OGRSpatialReference* srs, const char** options) {
    m_layer = data_source->CreateLayer("rings", srs, wkbPolygon, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'rings' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_id("id", OFTInteger);
    field_id.SetWidth(10);
    if (m_layer->CreateField(&field_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'id' on 'rings' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_nways("nways", OFTInteger);
    field_nways.SetWidth(6);
    if (m_layer->CreateField(&field_nways) != OGRERR_NONE ) {
        std::cerr << "Creating field 'nways' on 'rings' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_npoints("npoints", OFTInteger);
    field_npoints.SetWidth(8);
    if (m_layer->CreateField(&field_npoints) != OGRERR_NONE ) {
        std::cerr << "Creating field 'npoints' on 'rings' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_land("land", OFTInteger);
    field_land.SetWidth(1);
    if (m_layer->CreateField(&field_land) != OGRERR_NONE ) {
        std::cerr << "Creating field 'land' on 'rings' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_valid("valid", OFTInteger);
    field_valid.SetWidth(1);
    if (m_layer->CreateField(&field_valid) != OGRERR_NONE ) {
        std::cerr << "Creating field 'valid' on 'rings' layer failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

LayerRings::~LayerRings() {
    m_layer->CommitTransaction();
}

OGRErr LayerRings::commit() {
    return m_layer->CommitTransaction();
}

void LayerRings::add(OGRPolygon* polygon, int id, int nways, int npoints, LayerErrorPoints* layer_error_points) {
    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(polygon);
    feature->SetField("id", id);
    feature->SetField("nways", nways);
    feature->SetField("npoints", npoints);

    if (polygon->getExteriorRing()->isClockwise()) {
        feature->SetField("land", 1);
    } else {
        feature->SetField("land", 0);
    }

    if (polygon->IsValid()) {
        feature->SetField("valid", 1);
    } else if (layer_error_points) {
        /*
           When the polygon is invalid we find out what and where the problem is.
           This code is a bit strange because older versions of the GEOS library
           only export this information as a string. We parse the reason and
           point coordinates (of a self-intersection-point for instance) from
           this string and create a point in the error layer for it.

           The exportToGEOS() method on OGR geometries is not documented. Let's
           hope that it will always be available. We use the GEOSisValidReason()
           function from the GEOS C interface to get to the reason.
        */

        std::string reason = GEOSisValidReason(polygon->exportToGEOS());
        size_t left_bracket = reason.find('[');
        size_t right_bracket = reason.find(']');

        std::istringstream iss(reason.substr(left_bracket+1, right_bracket-left_bracket-1), std::istringstream::in);
        double x;
        double y;
        iss >> x;
        iss >> y;
        reason = reason.substr(0, left_bracket);

        OGRPoint* point = new OGRPoint();
        point->setX(x);
        point->setY(y);

        layer_error_points->add(point, id, reason.c_str());

        feature->SetField("valid", 0);
    }

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature in layer 'rings'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerPolygons::LayerPolygons(OGRDataSource* data_source, OGRSpatialReference* srs, const char** options) {
    m_layer = data_source->CreateLayer("polygons", srs, wkbPolygon, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'polygons' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_clockwise("clockwise", OFTInteger);
    field_clockwise.SetWidth(1);
    if (m_layer->CreateField(&field_clockwise) != OGRERR_NONE ) {
        std::cerr << "Creating field 'clockwise' on 'polygons' layer failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

LayerPolygons::~LayerPolygons() {
    m_layer->CommitTransaction();
}

OGRErr LayerPolygons::commit() {
    return m_layer->CommitTransaction();
}

void LayerPolygons::add(OGRPolygon* polygon, bool clockwise) {
    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(polygon);
    feature->SetField("clockwise", clockwise);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature in layer 'polygons'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

