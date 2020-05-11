# PDAL Julia Filter

Embeddable [Julia](https://julialang.org/) Filter stage for PDAL. Inspired by a similar effort for [Python](https://github.com/PDAL/python).

The recommended usage is through the Docker image
[cognitiveearth/pdal-julia](https://hub.docker.com/repository/docker/cognitiveearth/pdal-julia).

## Build

You can build the docker container with PDAL and this plugin,

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

You can test your Julia filters externally to PDAL using the [DevHarness.jl](examples/DevHarness.jl) file:

```
# Edit the example filter
vi examples/Example1.jl

# Run it using Julia standalone
julia examples/DevHarness.jl
```

## TODOs

- Expose metadata as a global to Julia fn
- Improve package management story, single scripts not very useful in isolation

