cmake_minimum_required(VERSION 2.8.12)
project(csm)
set(CMAKE_BUILD_TYPE Debug)

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
include_directories(include)

file(GLOB SOURCES "src/*.c")

add_library(csm SHARED ${SOURCES})

install(TARGETS csm DESTINATION /usr/lib)
