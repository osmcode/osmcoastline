#------------------------------------------------------------------------------
#
#  Makefile for OSMCoastline
#
#------------------------------------------------------------------------------

CXXFLAGS = -std=c++11 -O3
CXXFLAGS += -g -fno-omit-frame-pointer

CXXFLAGS += -Wall -Wextra -Wredundant-decls -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wno-long-long

CXXFLAGS_GEOS = $(shell geos-config --cflags)
CXXFLAGS_OGR  = $(shell gdal-config --cflags)

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

LIB_EXPAT = -lexpat -lbz2 -lz
LIB_PBF   = -lz -lpthread -lprotobuf-lite -losmpbf
LIB_GEOS  = $(shell geos-config --libs) -l geos_c
LIB_OGR   = $(shell gdal-config --libs)

PROGRAMS = osmcoastline_filter osmcoastline
ALLPROGRAMS = osmcoastline_ways $(PROGRAMS)

.PHONY: all clean

all: $(PROGRAMS)

osmcoastline_filter.o: osmcoastline_filter.cpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

osmcoastline_filter: osmcoastline_filter.o
	$(CXX) -o $@ $^ $(LIB_EXPAT) $(LIB_PBF)

osmcoastline_ways.o: osmcoastline_ways.cpp osmcoastline.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline_ways: osmcoastline_ways.o
	$(CXX) -o $@ $^ $(LIB_EXPAT) $(LIB_PBF) $(LIB_OGR) $(LIB_GEOS)

osmcoastline.o: osmcoastline.cpp osmcoastline.hpp coastline_ring.hpp coastline_ring_collection.hpp output_database.hpp output_layers.hpp options.hpp coastline_handlers.hpp coastline_polygons.hpp stats.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

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
	$(CXX) -o $@ $^ $(LIB_EXPAT) $(LIB_PBF) $(LIB_OGR) $(LIB_GEOS)

doc: doc/html/files.html

doc/html/files.html: *.hpp *.cpp
	doxygen >/dev/null

check:
	cppcheck --enable=all *.cpp

clean:
	rm -f *.o core $(ALLPROGRAMS)

