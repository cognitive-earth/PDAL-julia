cmake_minimum_required(VERSION 3.11.0)
project(pdal)

# macros for creating targets
include(${PROJECT_SOURCE_DIR}/cmake/macros.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/FindJulia.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE "Release")

# Python-finding settings

# find PDAL. Require 2.1+
find_package(PDAL 2.1 REQUIRED)


# Configure Julia for embedding
#
# https://docs.julialang.org/en/v1/manual/embedding/

# add_executable(Test test.cpp)
# target_include_directories(Test PUBLIC "$<BUILD_INTERFACE:${Julia_INCLUDE_DIRS}>")
# add_definitions(-DJULIA_ENABLE_THREADING)
# target_link_libraries(Test PUBLIC $<BUILD_INTERFACE:${Julia_LIBRARY}>)


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

# add_library(${extension} MODULE ${EXTENSION_SRC} libpdalpython)
# target_include_directories( ${extension}
#     PRIVATE
#     .
#     ${PDAL_INCLUDE_DIRS}
# )
    # ${Python3_INCLUDE_DIRS}
    # ${Python3_NumPy_INCLUDE_DIRS})

# target_link_libraries(${extension} ${PDAL_LIBRARIES})
# python_extension_module(${extension})

# install(TARGETS ${extension} LIBRARY DESTINATION ${PROJECT_NAME})

