using PackageCompiler
using Pkg

Pkg.add("TypedTables")

create_sysimage(:TypedTables, sysimage_path="pdal_jl_sys.so", precompile_execution_file="jl/Import.jl")

