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
#include <assert.h>

#include <ogrsf_frmts.h>
#include <geos_c.h>

#include <boost/lexical_cast.hpp>

#include <osmium/osm/types.hpp>

#include "osmcoastline.hpp"
#include "output_layers.hpp"
#include "srs.hpp"

extern SRS srs;

/***************************************************************/

OGRErr Layer::commit() {
    return m_layer->CommitTransaction();
}

/***************************************************************/

LayerErrorPoints::LayerErrorPoints(OGRDataSource* data_source, const char** options) :
    Layer()
{
    m_layer = data_source->CreateLayer("error_points", srs.out(), wkbPoint, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'error_points' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_osm_id("osm_id", OFTString);
    field_osm_id.SetWidth(10);
    if (m_layer->CreateField(&field_osm_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'osm_id' on 'error_points' layer failed.\n";
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

void LayerErrorPoints::add(OGRPoint* point, const char* error, osm_object_id_t osm_id) {
    srs.transform(point);

    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(point);
    feature->SetField("osm_id", boost::lexical_cast<std::string>(osm_id).c_str());
    feature->SetField("error", error);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature on layer 'error_points'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerErrorLines::LayerErrorLines(OGRDataSource* data_source, const char** options) :
    Layer()
{
    m_layer = data_source->CreateLayer("error_lines", srs.out(), wkbLineString, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'error_lines' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_osm_id("osm_id", OFTString);
    field_osm_id.SetWidth(10);
    if (m_layer->CreateField(&field_osm_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'osm_id' on 'error_lines' layer failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_error("error", OFTString);
    field_error.SetWidth(16);
    if (m_layer->CreateField(&field_error) != OGRERR_NONE ) {
        std::cerr << "Creating field 'error' on 'error_lines' layer failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

void LayerErrorLines::add(OGRLineString* linestring, const char* error, osm_object_id_t osm_id) {
    srs.transform(linestring);

    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(linestring);
    feature->SetField("osm_id", boost::lexical_cast<std::string>(osm_id).c_str());
    feature->SetField("error", error);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature on layer 'error_lines'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerRings::LayerRings(OGRDataSource* data_source, const char** options) :
    Layer()
{
    m_layer = data_source->CreateLayer("rings", srs.out(), wkbPolygon, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'rings' failed.\n";
        exit(return_code_fatal);
    }

    OGRFieldDefn field_osm_id("osm_id", OFTString);
    field_osm_id.SetWidth(10);
    if (m_layer->CreateField(&field_osm_id) != OGRERR_NONE ) {
        std::cerr << "Creating field 'osm_id' on 'rings' layer failed.\n";
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

    OGRFieldDefn field_fixed("fixed", OFTInteger);
    field_fixed.SetWidth(1);
    if (m_layer->CreateField(&field_fixed) != OGRERR_NONE ) {
        std::cerr << "Creating field 'fixed' on 'rings' layer failed.\n";
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

void LayerRings::add(OGRPolygon* polygon, int osm_id, int nways, int npoints, bool fixed, LayerErrorPoints* layer_error_points) {
    srs.transform(polygon);

    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(polygon);
    feature->SetField("osm_id", osm_id);
    feature->SetField("nways", nways);
    feature->SetField("npoints", npoints);
    feature->SetField("fixed", fixed ? 0 : 1);

    if (polygon->getExteriorRing()->isClockwise()) {
        feature->SetField("land", 1);
    } else {
        feature->SetField("land", 0);
    }

    if (polygon->IsValid()) {
        feature->SetField("valid", 1);
    } else {
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
        point->assignSpatialReference(polygon->getSpatialReference());
        point->setX(x);
        point->setY(y);

        if (reason == "Self-intersection") {
            reason = "self_intersection";
        }
        layer_error_points->add(point, reason.c_str(), osm_id);

        feature->SetField("valid", 0);
    }

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature in layer 'rings'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerPolygons::LayerPolygons(OGRDataSource* data_source, const char** options, const char* name) :
    Layer(),
    m_name(name)
{
    m_layer = data_source->CreateLayer(name, srs.out(), wkbPolygon, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer '" << name << "' failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

void LayerPolygons::add(OGRPolygon* polygon) {
    srs.transform(polygon);

    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(polygon);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature in layer '" << m_name << "'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

/***************************************************************/

LayerLines::LayerLines(OGRDataSource* data_source, const char** options) :
    Layer()
{
    m_layer = data_source->CreateLayer("lines", srs.out(), wkbLineString, const_cast<char**>(options));
    if (m_layer == NULL) {
        std::cerr << "Creating layer 'lines' failed.\n";
        exit(return_code_fatal);
    }

    m_layer->StartTransaction();
}

void LayerLines::add(OGRLineString* linestring) {
    srs.transform(linestring);

    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());

    feature->SetGeometryDirectly(linestring);

    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature in layer 'lines'.\n";
        exit(return_code_fatal);
    }

    OGRFeature::DestroyFeature(feature);
}

