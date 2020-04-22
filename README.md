# PDAL Julia Filter

Embeddable [Julia](https://julialang.org/) Filter stage for PDAL. Inspired by a similar effort for [Python](https://github.com/PDAL/python).

## Build

You will need CMake, a modern C++ compiler, PDAL and Julia installed (including headers and dev libraries).

Build the shared lib,

```
cd pdal
cmake .
make
```

Then link it when building PDAL.

You can then run the tests,

```
./julia_filter_test
```

## Usage

When PDAL is built with this plugin, filter stages can be written in Julia.

```json
[
  {
    "type": "readers.las",
    "filename": "input.las"
  },
  {
    "type": "filters.julia",
    "source": "module MyModule\n function myfunc(in)\nreturn in\nend\nend\n",
    "module": "MyModule",
    "function": "myfunc"
  },
  {
    "type": "writers.las",
    "filename": "output.las"
  }
]
```

## Julia Function Interface

The aim is to expose a modern Julia interface for dealing with PointCloud data, so the provided Julia function
must have the the following interface:

```
TODO
```

A point cloud is represented as a Table from TypedTables.jl where the position column contains 3D points.

We make the following packages available by default

- https://github.com/visr/LasIO.jl
- https://github.com/FugroRoames/RoamesGeometry.jl
- https://github.com/JuliaData/TypedTables.jl


## Progress

- [x] Compile PDAL filter with access to Julia headers and runtime
- [x] Embedded Julia interpreter inside PDAL filter
- [x] Compile script once, run for each PointCloudView
- [x] Runnable unit test for new PDAL filter
- [x] POC of passing Point Cloud data into Julia script (X,Y,Z dims at least)
- []  Design interface for Julia scripts
- []  Include useful Julia libraries for use by submitted script
- []  Wrapper (in Julia or C++?) to convert from array of c++ arrays (one per dim) to rich Julia type and pass into function
- []  Wrapper (in Julia or C++?) to convert from rich Julia return type to array of c++ arrays
- []  Build Alpine and Debian docker images

