using TypedTables
using RoamesGeometry

if length(ARGS) == 0
  println("Please specify a file in the 'examples' folder")
  exit(1)
end

include(ARGS[1])

t = time()
if length(ARGS) == 2
  # println("Using $(ARGS[2]) as LAS input")
  # Load LAS file into RoamesGeometry format
  input = RoamesGeometry.load_las(ARGS[2])

  # TODO: Make color optional

  # Convert into PDAL-julia Table format
  input = TypedTables.FlexTable(
      X = map(p -> p[1], input.position),
      Y = map(p -> p[2], input.position),
      Z = map(p -> p[3], input.position),
      Intensity = input.intensity,
      Classification = input.classification,
      R = map(p -> p.r, input.color),
      G = map(p -> p.g, input.color),
      B = map(p -> p.b, input.color),
      RadialDensity= zeros(length(input.position))
   )
else
  input = TypedTables.FlexTable(
      X = [1.0, 2.0, 3.0, 4.0, 5.0],
      Y = [1.0, 2.0, 3.0, 4.0, 5.0],
      Z = [1.0, 2.0, 3.0, 4.0, 5.0]
  )
end

println("Test input:");
println(input)
println("===========================");

println("Running filter...");
output = TestModule.runFilter(input)

println("Test output:");
println(output)
println("===========================");
