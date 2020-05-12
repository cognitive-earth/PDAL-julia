#
# Adds a new dimension to the Point Cloud "Num_Points_In_Radius" which is the number of 
# points with 50 "units" of each point.
#
module TestModule

  using RoamesGeometry
  using AcceleratedArrays
  using StructArrays
  using StaticArrays
  using TypedTables

  # The radius to look in
  radius = 50

  function runFilter(ins)

    # Create accelerated array for spatial lookups
    positions = accelerate(
      StructArray(SVector{3, Float64}(r.X, r.Y, r.Z) for r in ins),
      GridIndex;
      spacing = 5.0
    )

    # Per point operation to be run inside a map
    function find_nearby_points(point, points)
      padded_bbox = RoamesGeometry.pad(RoamesGeometry.boundingbox(point), radius)
      return RoamesGeometry.findall(RoamesGeometry.in(padded_bbox), points)
    end

    nearby_points_arr = map(position -> length(find_nearby_points(position, positions)), positions)
    return merge(columns(ins), (Num_Points_In_Radius=nearby_points_arr,))
  end

end # module
