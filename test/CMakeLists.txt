#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSMCoastline Tests
#
#-----------------------------------------------------------------------------

file(GLOB TEST_SCRIPTS t/*.sh)

foreach(file ${TEST_SCRIPTS})
    string(REGEX REPLACE "^.*/test-([0-9]+).sh$" "\\1" tid ${file})

    message(STATUS "Adding test ${tid} (${file})")
    add_test(NAME test-${tid}
             COMMAND ${file} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${tid})
endforeach()


#-----------------------------------------------------------------------------
