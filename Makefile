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

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

LDFLAGS = -L/usr/local/lib -lexpat -lpthread

LIB_PROTOBUF = -lz -lprotobuf-lite -losmpbf
LIB_GEOS     = $(shell geos-config --libs) -l geos_c
LIB_OGR      = $(shell gdal-config --libs)

PROGRAMS = osmcoastline

.PHONY: all clean

all: $(PROGRAMS)

osmcoastline.o: osmcoastline.cpp coastline_ring.hpp output.hpp output_layers.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

output.o: output.cpp output.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

output_layers.o: output_layers.cpp output_layers.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

coastline_ring.o: coastline_ring.cpp coastline_ring.hpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_OGR) -o $@ $<

osmcoastline: osmcoastline.o coastline_ring.o output.o output_layers.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIB_PROTOBUF) $(LIB_OGR) $(LIB_GEOS)

clean:
	rm -f *.o core $(PROGRAMS)

