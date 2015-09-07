cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "Meteor++")
set(PROJECT_DESCRIPTION "Meteor DDP & Minimongo implementation in C++.")
set(PROJECT_VENDOR "Almighty Couch")
set(PROJECT_WEBSITE "http://github.com/almightycouch")
set(PROJECT_MAINTAINER "Mario Flach <m.flach@almightycouch.org>")

project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Boost COMPONENTS random system REQUIRED)
find_package(Ejdb REQUIRED)

#include_directories("${Boost_INCLUDE_DIRS}")
#include_directories("${Ejdb_INCLUDE_DIRS}")

include_directories("${PROJECT_SOURCE_DIR}/include")
file(GLOB METEORPP_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(meteorpp SHARED ${METEORPP_SRC})
target_link_libraries(meteorpp ${Boost_LIBRARIES} ${Ejdb_LIBRARIES})
