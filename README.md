# PDAL Julia Filter

Embeddable [Julia](https://julialang.org/) Filter stage for PDAL. Inspired by a similar effort for [Python](https://github.com/PDAL/python).

## Build

Build the shared lib,

```
cd pdal
cmake .
make
```

Then link it when building PDAL.


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
