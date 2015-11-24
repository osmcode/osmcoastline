#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  OSMCoastline
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(osmcoastline)

set(OSMCOASTLINE_VERSION_MAJOR 2)
set(OSMCOASTLINE_VERSION_MINOR 1)
set(OSMCOASTLINE_VERSION_PATCH 1)

set(OSMCOASTLINE_VERSION
    ${OSMCOASTLINE_VERSION_MAJOR}.${OSMCOASTLINE_VERSION_MINOR}.${OSMCOASTLINE_VERSION_PATCH})

add_definitions(-DOSMCOASTLINE_VERSION="${OSMCOASTLINE_VERSION}")

set(AUTHOR "Jochen Topf <jochen@topf.org>")


#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

include_directories(include)

find_package(Osmium COMPONENTS io gdal)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})

if(MSVC)
    find_path(GETOPT_INCLUDE_DIR getopt.h)
    find_library(GETOPT_LIBRARY NAMES wingetopt)
    if(GETOPT_INCLUDE_DIR AND GETOPT_LIBRARY)
        include_directories(${GETOPT_INCLUDE_DIR})
        list(APPEND OSMIUM_LIBRARIES ${GETOPT_LIBRARY})
    else()
        set(GETOPT_MISSING 1)
    endif()
endif()


#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
if(MSVC)
    set(USUAL_COMPILE_OPTIONS "/Ox")
else()
    set(USUAL_COMPILE_OPTIONS "-O3 -g")
endif()

if(WIN32)
    add_definitions(-DWIN32 -D_WIN32 -DMSWIN32 -DBGDWIN32
                    -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0600)
endif()

set(CMAKE_CXX_FLAGS_DEV "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------
set(CMAKE_CONFIGURATION_TYPES "Debug Release RelWithDebInfo MinSizeRel Dev")

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror -fno-omit-frame-pointer)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB ALL_CODE *.cpp)

    set(CPPCHECK_FILES ${ALL_CODE})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------
#
#  Optional "man" target to generate man pages
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for pandoc")
find_program(PANDOC pandoc)

function(add_man_page _section _name)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/man/man${_section})
    set(_output_file ${CMAKE_BINARY_DIR}/man/man${_section}/${_name}.${_section})
    set(_source_file ${CMAKE_SOURCE_DIR}/man/${_name}.md)
    set(_install_dir "share/man/man{$_section}")
    string(TOUPPER ${_name} _name_upcase)
    add_custom_command(OUTPUT ${_output_file}
        COMMAND ${PANDOC}
            ${PANDOC_MAN_OPTIONS}
            --variable "title=${_name_upcase}"
            --variable "section=${_section}"
            -o ${_output_file}
            ${_source_file}
        DEPENDS ${_source_file} man/manpage.template
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building manpage ${_name}.${_section}"
        VERBATIM)
    set(ALL_MAN_PAGES "${ALL_MAN_PAGES};${_output_file}" PARENT_SCOPE)
endfunction()


if(PANDOC)
    message(STATUS "Looking for pandoc - found")
    message(STATUS "  Manual pages will be built")
    set(PANDOC_MAN_OPTIONS
        -s
        -t man
        --template ${CMAKE_CURRENT_SOURCE_DIR}/man/manpage.template
        --variable "description=osmcoastline/${OSMCOASTLINE_VERSION}"
        --variable "version=${OSMCOASTLINE_VERSION}"
        --variable "author=${AUTHOR}"
    )
    set(PANDOC_HTML_OPTIONS -s -t html)

    add_man_page(1 osmcoastline)
    add_man_page(1 osmcoastline_filter)
    add_man_page(1 osmcoastline_readmeta)
    add_man_page(1 osmcoastline_segments)
    add_man_page(1 osmcoastline_ways)

    install(DIRECTORY ${CMAKE_BINARY_DIR}/man DESTINATION share)

    add_custom_target(man ALL DEPENDS ${ALL_MAN_PAGES})
else()
    message(STATUS "Looking for pandoc - not found")
    message(STATUS "  Manual pages will not be built")
endif(PANDOC)


#-----------------------------------------------------------------------------

find_library(GEOS_C_LIBRARIES NAMES geos_c)

add_definitions(${OSMIUM_WARNING_OPTIONS})

add_subdirectory(src)

configure_file(
    ${PROJECT_SOURCE_DIR}/runtest.sh.in
    ${PROJECT_BINARY_DIR}/runtest.sh
)

configure_file(
    ${PROJECT_SOURCE_DIR}/coastline_sqlite.qgs
    ${PROJECT_BINARY_DIR}/coastline_sqlite.qgs
)

configure_file(
    ${PROJECT_SOURCE_DIR}/coastline_sources.qgs
    ${PROJECT_BINARY_DIR}/coastline_sources.qgs
)

configure_file(
    ${PROJECT_SOURCE_DIR}/osmcoastline_readmeta
    ${PROJECT_BINARY_DIR}/osmcoastline_readmeta
    @ONLY
)
install(PROGRAMS osmcoastline_readmeta DESTINATION bin)


#-----------------------------------------------------------------------------
#
#  Documentation
#
#-----------------------------------------------------------------------------

add_subdirectory(doc)


#-----------------------------------------------------------------------------
#
#  Packaging
#
#-----------------------------------------------------------------------------

set(CPACK_PACKAGE_VERSION_MAJOR ${OSMCOASTLINE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OSMCOASTLINE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OSMCOASTLINE_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()

include(CPack)


#-----------------------------------------------------------------------------
