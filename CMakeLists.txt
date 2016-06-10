project(csm)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CSM_MAJOR_VERSION "0")
set(CSM_MINOR_VERSION "0")
set(CSM_PATCH_VERSION "0")

set(CSM_VERSION "${CSM_MAJOR_VERSION}.${CSM_MINOR_VERSION}.${CSM_PATCH_VERSION}")

set(CMAKE_BUILD_TYPE Debug)
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()
add_test(NAME check_csm COMMAND check_csm)
