using Pkg

Pkg.add("PackageCompiler")
Pkg.add("TypedTables")

using PackageCompiler
create_sysimage(:TypedTables, sysimage_path="pdal/pdal_jl_sys.so", precompile_execution_file="jl/Import.jl")

