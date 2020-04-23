cmake_minimum_required(VERSION 3.11.0)
project(pdal)

include(${PROJECT_SOURCE_DIR}/cmake/macros.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/FindJulia.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE "Release")

##################################################################################
#
# PDAL (Require 2.1+)
#
##################################################################################
find_package(PDAL 2.1 REQUIRED)

##################################################################################
#
# GOOGLETEST
#
##################################################################################

# Download and unpack googletest at configure time
configure_file(test/gtest/CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)


##################################################################################
#
# Embed Julia (https://docs.julialang.org/en/v1/manual/embedding/)
#
##################################################################################
add_definitions(-DJULIA_ENABLE_THREADING)

set(JULIA_SYS_PATH "${Julia_LIBRARY_DIR}/julia")

# Configure julia filter as PDAL extension
PDAL_JULIA_ADD_PLUGIN(julia_filter filter julia
  FILES
    ./filters/JuliaFilter.cpp
    ./filters/JuliaFilter.hpp
    ./jlang/Script.cpp
    ./jlang/Invocation.cpp
  LINK_WITH
    ${PDAL_LIBRARIES}
     $<BUILD_INTERFACE:${Julia_LIBRARY}>
  SYSTEM_INCLUDES
    ${PDAL_INCLUDE_DIRS}
    "$<BUILD_INTERFACE:${Julia_INCLUDE_DIRS}>"
)

# Unit tests for julia filter
PDAL_JULIA_ADD_TEST(julia_filter_test
  FILES
    ./test/JuliaFilterTest.cpp
    ./test/Support.cpp
  LINK_WITH
    ${julia_filter}
    ${PDAL_LIBRARIES}
    $<BUILD_INTERFACE:${Julia_LIBRARY}>
  SYSTEM_INCLUDES
    ${PDAL_INCLUDE_DIRS}
    "$<BUILD_INTERFACE:${Julia_INCLUDE_DIRS}>"
)

