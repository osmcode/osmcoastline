#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSMCoastline Tests
#
#-----------------------------------------------------------------------------

file(GLOB TEST_SCRIPTS t/*.sh)

list(SORT TEST_SCRIPTS)

foreach(file ${TEST_SCRIPTS})
    string(REGEX REPLACE "^.*/(.+).sh$" "\\1" tid ${file})

    message(STATUS "Adding test ${tid}")
    add_test(NAME test-${tid}-4326
             COMMAND ${file} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${tid} 4326)
    add_test(NAME test-${tid}-3857
             COMMAND ${file} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${tid} 3857)
endforeach()


#-----------------------------------------------------------------------------
