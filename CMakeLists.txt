cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "Meteor++")
set(PROJECT_DESCRIPTION "Meteor DDP & Minimongo implementation in C++.")
set(PROJECT_VENDOR "Almighty Couch")
set(PROJECT_WEBSITE "http://github.com/almightycouch")
set(PROJECT_MAINTAINER "Mario Flach <m.flach@almightycouch.org>")

project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include(FindPkgConfig)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${PROJECT_SOURCE_DIR}/ejdb/dist/lib/pkgconfig" )

if(APPLE)
    set(CMAKE_MACOSX_RPATH 0)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl")
endif()

# load boost libraries from system
find_package(Boost COMPONENTS random system REQUIRED)

# load libejdb from submodule
pkg_check_modules(Ejdb REQUIRED libejdb)

# libejdb library path
link_directories(${Ejdb_LIBRARY_DIRS})

# third-party include dirs
include_directories(${Boost_INCLUDE_DIRS} ${Ejdb_INCLUDE_DIRS})

# meteorpp include dirs
include_directories("${PROJECT_SOURCE_DIR}/include")

# sources
file(GLOB METEORPP_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB METEORPP_UNITTEST_SRC "${PROJECT_SOURCE_DIR}/tests/*.cpp")

# create libmeteorpp
add_library(meteorpp SHARED ${METEORPP_SRC})

# link against boost_random, boost_system & ejdb
target_link_libraries(meteorpp ${Boost_LIBRARIES} ${Ejdb_LIBRARIES})

#create unit tests
add_executable(tests EXCLUDE_FROM_ALL ${METEORPP_SRC} ${METEORPP_UNITTEST_SRC})

# link against boost_random, boost_system & ejdb
target_link_libraries(tests ${Boost_LIBRARIES} ${Ejdb_LIBRARIES})

# compile with profiling and coverage flags
set_target_properties(tests PROPERTIES COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
set_target_properties(tests PROPERTIES    LINK_FLAGS "-fprofile-arcs -ftest-coverage")
