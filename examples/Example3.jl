#
# Simple Filter stage to remove all points with less than 420 as a Z value
#
module TestModule

  using TypedTables

  function runFilter(ins)
    return filter(p -> p.Z > 420.0, ins)
  end

end # module
