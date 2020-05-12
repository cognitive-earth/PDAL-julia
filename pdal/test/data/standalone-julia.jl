module Test

  using RoamesGeometry
  using TypedTables

  function go(file::String,dist::Float64)
    t = time()
    pc = RoamesGeometry.load_las(file, spacing=5.0)
    pc = Table(merge(columns(pc),(Num_Points_In_Radius=zeros(length(pc)),)))
    println("Load timing $(time()-t)")

    for run in 1:10
      t = time()
      for point in pc
        padded_bbox= RoamesGeometry.pad(RoamesGeometry.boundingbox(point.position), dist / 2)
        points_in_radius = RoamesGeometry.findall(RoamesGeometry.in(padded_bbox), pc.position)
        point = merge(point, (; :Num_Points_In_Radius => length(points_in_radius)))
      end
      println("Loop timing $(time()-t) $run")
    end
  end

end

dist = 50.0
Test.go(ARGS[1], dist)
