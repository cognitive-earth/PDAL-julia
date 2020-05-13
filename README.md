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
- https://github.com/FugroRoames/RoamesGeometry.jl
- https://github.com/andyferris/AcceleratedArrays.jl
- https://github.com/JuliaArrays/StructArrays.jl
- https://github.com/JuliaArrays/StaticArrays.jl

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

### Performance Characteristics

See the `/benchmark.sh` script for code to replicate. This is just an initial investigation into the relative performance
of a filter written in Julia vs C++. I re-implemented the
[Radial Density Filter](https://pdal.io/stages/filters.radialdensity.html) from PDAL in Julia:

```julia
#
# Julia version of https://pdal.io/stages/filters.radialdensity.html
#
module TestModule

  using RoamesGeometry
  using AcceleratedArrays
  using StructArrays
  using StaticArrays
  using TypedTables

  # The radius to look in
  radius = 5.0
  factor = 1.0 / ((4.0 / 3.0) * 3.14159 * (radius * radius * radius))

  function runFilter(ins)
    # Create accelerated array for spatial lookups
    positions = accelerate(
      StructArray(SVector{3, Float64}(r.X, r.Y, r.Z) for r in ins),
      GridIndex;
      spacing = radius * 1.5 # TODO: What is a good ratio here?
    )

    # Per point operation
    function find_nearby_points(point, points)
      padded_sphere = RoamesGeometry.boundingbox(Sphere(point, radius))
      return RoamesGeometry.findall(RoamesGeometry.in(padded_sphere), points)
    end

    for i in 1:length(positions)
      position = positions[i]
      ins[i] = merge(ins[i], (;RadialDensity=factor * length(find_nearby_points(position, positions))))
    end

    return ins;
  end

end # module
```

Then ran both filters against two different point cloud inputs; `autzen.las` (4.9k) and `1.2-with-color.las` (36k). The idea
was to determine roughly the overhead of starting up the Julia interpreter vs the actual processing task by using different
file sizes.

#### Results

|  | PDAL | Julia |  |  |
|----------------|-------|-------|:-:|---|
| 1.2-with-color | 0.35s | 3.09s |  |  |
| Autzen | 0.19s | 3.49s |  |  |
| Diff | 0.16s | 0.40s |  |  |

From these results it seems that the startup overhead is dominating the performance difference.


