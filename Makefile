#------------------------------------------------------------------------------
#
#  Makefile for osmcoastline
#
#------------------------------------------------------------------------------

CXX = g++
#CXX = clang

#CXXFLAGS = -g
CXXFLAGS = -O3

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

PROGRAMS = osmcoastline

.PHONY: all clean

all: $(PROGRAMS)

osmcoastline.o: osmcoastline.cpp osmcoastline.hpp coastline_ring.hpp output_database.hpp output_layers.hpp options.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) $(CXXFLAGS_OGR) -o $@ $<

output_database.o: output_database.cpp output_database.hpp output_layers.hpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

output_layers.o: output_layers.cpp output_layers.hpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

coastline_ring.o: coastline_ring.cpp coastline_ring.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline: osmcoastline.o coastline_ring.o output_database.o output_layers.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_OGR) $(LIB_GEOS) $(LIB_XML2)

clean:
	rm -f *.o core $(PROGRAMS)

