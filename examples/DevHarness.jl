using TypedTables
include("./Example1.jl")

input = TypedTables.FlexTable(
    X = [1.0, 2.0, 3.0, 4.0, 5.0],
    Y = [1.0, 2.0, 3.0, 4.0, 5.0],
    Z = [1.0, 2.0, 3.0, 4.0, 5.0]
)

println("Test input:");
println(input)

output = TestModule.fff(input)

println("Test output:");
println(output)
