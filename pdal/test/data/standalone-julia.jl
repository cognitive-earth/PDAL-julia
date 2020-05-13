module Test

  using RoamesGeometry
  using TypedTables

  radius = 50.0
  factor = 1.0 / ((4.0 / 3.0) * 3.14159 * (radius * radius * radius))

  function go(file::String)
    t = time()
    pc = RoamesGeometry.load_las(file, spacing=5.0)
    pc = Table(merge(columns(pc),(Num_Points_In_Radius=zeros(length(pc)),)))
    println("Load timing $(time()-t)")

    for run in 1:10
      t = time()
      for point in pc
        padded_sphere = RoamesGeometry.boundingbox(Sphere(point.position, radius))
        points_in_radius = RoamesGeometry.findall(RoamesGeometry.in(padded_sphere), pc.position)
        point = merge(point, (; :RadialDensity => factor * length(points_in_radius)))
      end
      println("Loop timing $(time()-t) $run")
    end
  end

end

Test.go(ARGS[1])
