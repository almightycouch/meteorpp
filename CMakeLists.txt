cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "Meteor++")
set(PROJECT_DESCRIPTION "Meteor DDP & Minimongo implementation in C++.")
set(PROJECT_VENDOR "Almighty Couch")
set(PROJECT_WEBSITE "http://github.com/almightycouch")
set(PROJECT_MAINTAINER "Mario Flach <m.flach@almightycouch.org>")

project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
if(APPLE)
    set(CMAKE_MACOSX_RPATH 0)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl")
endif()

# third-party libraries
find_package(Boost COMPONENTS random system REQUIRED)
find_package(Ejdb REQUIRED)

# include dirs
include_directories("${PROJECT_SOURCE_DIR}/include")

# sources
file(GLOB METEORPP_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")

# create libmeteorpp
add_library(meteorpp SHARED ${METEORPP_SRC})

# link with boost_random, boost_system & ejdb
target_link_libraries(meteorpp ${Boost_LIBRARIES} ${Ejdb_LIBRARIES})
