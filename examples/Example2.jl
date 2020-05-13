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
