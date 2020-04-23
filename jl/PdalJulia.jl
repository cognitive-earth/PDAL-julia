module PdalJulia

  using TypedTables

  function extractString(ptrArray, i, numDims)
      if i == numDims
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ))
      else 
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ), ptrArray[i + 1] - ptrArray[i] )
      end
  end

  function runStage(args...)
    numDims = length(args) - 3
    ptrArray = args[length(args) - 2] # 3nd last arg is an array of pointers to the names of each dim
    userFn = args[length(args)] # last arg is the user supplied function

    # Build a dictionary of dimName:dimValArray
    dims = Dict{Symbol,Array}()
    for i = 1:numDims
        dimName = Symbol(extractString(ptrArray, i, numDims))
        dimValArray = args[i]
        dims[dimName] = dimValArray
    end

    # Convert the Dict into a named tuple
    nt = NamedTuple{Tuple(keys(dims))}(values(dims))

    # Convert to a TypedTable
    tbl = Table(nt)

    # Run the user-supplied function on the input data
    ret = userFn(tbl)

    # Convert the TypedTable back into a format that is readable from C++
    return unwrapRet(ret)
  end

  function unwrapRet(ret)
    result = []
    dims = []
    for colname in TypedTables.columnnames(ret)
      col =Base.getproperty(ret, colname)

      push!(dims, string(colname))
      push!(result, col)
    end

    # Add the dim names in order as the last element in the result array passed back to C++
    push!(result, dims)

    return result
  end

end # module
