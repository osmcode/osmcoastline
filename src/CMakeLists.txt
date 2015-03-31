#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSMCoastline
#
#-----------------------------------------------------------------------------

add_executable(osmcoastline osmcoastline.cpp coastline_ring.cpp coastline_ring_collection.cpp coastline_polygons.cpp output_database.cpp output_layers.cpp srs.cpp options.cpp)
target_link_libraries(osmcoastline ${OSMIUM_IO_LIBRARIES} ${GDAL_LIBRARIES} ${GEOS_C_LIBRARIES})
install(TARGETS osmcoastline DESTINATION bin)

add_executable(osmcoastline_filter osmcoastline_filter.cpp)
target_link_libraries(osmcoastline_filter ${OSMIUM_IO_LIBRARIES})
install(TARGETS osmcoastline_filter DESTINATION bin)

add_executable(osmcoastline_ways osmcoastline_ways.cpp osmcoastline.hpp)
target_link_libraries(osmcoastline_ways ${OSMIUM_IO_LIBRARIES} ${GDAL_LIBRARIES})
install(TARGETS osmcoastline_ways DESTINATION bin)

