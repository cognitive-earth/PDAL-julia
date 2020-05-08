# PDAL Julia Filter

Embeddable [Julia](https://julialang.org/) Filter stage for PDAL. Inspired by a similar effort for [Python](https://github.com/PDAL/python).

## Build

It is recommended to use the docker container built with PDAL and this plugin,

```
docker build scripts/docker/alpine -t pdal-julia
```

run a test pipeline

```
docker run pdal-julia pdal translate ../test/data/las/1.2-with-color.las julia-out.las julia \
    --filters.julia.script=../../PDAL-julia/pdal/test/data/test1.jl \
    --filters.julia.module="TestModule" \
    --filters.julia.function="fff"

# Check the output is valid
./bin/pdal info julia-out.las
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
function (TypedTable) -> TypedTable
```

A point cloud is represented as a Table from TypedTables.jl where the X,Y,Z columns contains 3D point positions.

We make the following packages available by default

- https://github.com/JuliaData/TypedTables.jl


## Progress

- [x] Compile PDAL filter with access to Julia headers and runtime
- [x] Embedded Julia interpreter inside PDAL filter
- [x] Compile script once, run for each PointCloudView
- [x] Runnable unit test for new PDAL filter
- [x] POC of passing Point Cloud data into Julia script (X,Y,Z dims at least)
- [x]  Design interface for Julia scripts
- [x]  Include useful Julia libraries for use by submitted script
- [x]  Wrapper (in Julia) to convert from array of c++ arrays (one per dim) to rich Julia type and pass into function
- [x]  Wrapper (in Julia/C++) to convert from rich Julia return type to array of c++ arrays
- [x]  Pass results of Julia filter back into PDAL interface
- []  Expose metadata as a global to Julia fn
- [x]  Support data types other than floats
- [x]  Run as a filter in PDAL
- []  Reasonable test coverage
- [x]  Build Alpine docker image

