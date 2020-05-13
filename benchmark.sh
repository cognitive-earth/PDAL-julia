
time pdal translate ./pdal/test/data/1.2-with-color.las ./pdal-out.csv radialdensity \
  --filters.radialdensity.radius=5 \
  --writers.text.order="X,Y,Z,RadialDensity" \
  --writers.text.keep_unspecified=false

time pdal translate ./pdal/test/data/1.2-with-color.las ./julia-out.csv julia \
  --filters.julia.script=./examples/Example2.jl \
  --filters.julia.module="TestModule" \
  --filters.julia.function="runFilter" \
  --filters.julia.add_dimension="RadialDensity" \
  --writers.text.order="X,Y,Z,RadialDensity" \
  --writers.text.keep_unspecified=false



time pdal translate ./pdal/test/data/autzen.las ./pdal-out.csv radialdensity \
  --filters.radialdensity.radius=5 \
  --writers.text.order="X,Y,Z,RadialDensity" \
  --writers.text.keep_unspecified=false

time pdal translate ./pdal/test/data/autzen.las ./julia-out.csv julia \
  --filters.julia.script=./examples/Example2.jl \
  --filters.julia.module="TestModule" \
  --filters.julia.function="runFilter" \
  --filters.julia.add_dimension="RadialDensity" \
  --writers.text.order="X,Y,Z,RadialDensity" \
  --writers.text.keep_unspecified=false





