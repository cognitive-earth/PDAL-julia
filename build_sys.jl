using Pkg

Pkg.add("PackageCompiler")
Pkg.add("TypedTables")
Pkg.add(PackageSpec(url="https://github.com/FugroRoames/RoamesGeometry.jl"))
Pkg.add("AcceleratedArrays")
Pkg.add("StructArrays")
Pkg.add("StaticArrays")

using PackageCompiler
create_sysimage(:TypedTables, sysimage_path="pdal/pdal_jl_sys.so", precompile_execution_file="jl/Import.jl")

