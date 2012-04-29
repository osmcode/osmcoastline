#------------------------------------------------------------------------------
#
#  Makefile for osmcoastline
#
#------------------------------------------------------------------------------

CXX = g++
#CXX = clang

#CXXFLAGS = -g
CXXFLAGS = -O3 -g

CXXFLAGS += -Wall -Wextra -Wredundant-decls -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wno-long-long

CXXFLAGS_GEOS = -DOSMIUM_WITH_GEOS $(shell geos-config --cflags)
CXXFLAGS_OGR  = -DOSMIUM_WITH_OGR $(shell gdal-config --cflags)
CXXFLAGS_LIBXML2 = -DOSMIUM_WITH_OUTPUT_OSM_XML $(shell xml2-config --cflags)

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

LDFLAGS = -L/usr/local/lib -lexpat -lpthread

LIB_PROTOBUF = -lz -lprotobuf-lite -losmpbf
LIB_GEOS     = $(shell geos-config --libs) -l geos_c
LIB_OGR      = $(shell gdal-config --libs)
LIB_XML2     = $(shell xml2-config --libs)

PROGRAMS = osmcoastline_extract osmcoastline_intersections osmcoastline_ways osmcoastline

.PHONY: all clean

all: $(PROGRAMS)

osmcoastline_extract.o: osmcoastline_extract.cpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) -o $@ $<

osmcoastline_extract: osmcoastline_extract.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_XML2)

osmcoastline_ways.o: osmcoastline_ways.cpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline_ways: osmcoastline_ways.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_OGR) $(LIB_GEOS) $(LIB_XML2)

osmcoastline_intersections.o: osmcoastline_intersections.cpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline_intersections: osmcoastline_intersections.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_OGR) $(LIB_GEOS) $(LIB_XML2)

osmcoastline.o: osmcoastline.cpp osmcoastline.hpp coastline_ring.hpp coastline_ring_collection.hpp output_database.hpp output_layers.hpp options.hpp coastline_handlers.hpp coastline_polygons.hpp stats.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) $(CXXFLAGS_OGR) -o $@ $<

srs.o: srs.cpp srs.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

options.o: options.cpp options.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

output_database.o: output_database.cpp output_database.hpp output_layers.hpp osmcoastline.hpp options.hpp stats.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

output_layers.o: output_layers.cpp output_layers.hpp osmcoastline.hpp srs.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

coastline_ring.o: coastline_ring.cpp coastline_ring.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

coastline_ring_collection.o: coastline_ring_collection.cpp coastline_ring_collection.hpp output_database.hpp coastline_ring.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

coastline_polygons.o: coastline_polygons.cpp coastline_polygons.hpp output_database.hpp osmcoastline.hpp srs.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline: osmcoastline.o coastline_ring.o coastline_ring_collection.o coastline_polygons.o output_database.o output_layers.o srs.o options.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_OGR) $(LIB_GEOS) $(LIB_XML2)

doc: doc/html/files.html

doc/html/files.html: *.hpp *.cpp
	doxygen >/dev/null

clean:
	rm -f *.o core $(PROGRAMS)

