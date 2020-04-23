module PdalJulia

  using TypedTables

  #
  # The main runtime for interfacing between the PDAL C++ Stage and the user-supplied Julia fn.
  #
  # This function is passed a variable number of arguments,
  #
  # 1..N-3 => Array for each dimension in the PointCloud
  # N-2    => Array of pointers to the start of each string in the next argument
  # N-1    => Array of chars containing the names of all dimensions
  # N      => The user-defined function. It should be of the type: (Table -> Table)
  #
  # The execution of the stage consists of converting the input arguments into a TypedTable representation,
  # running the user-supplied function with the TypedTable as its only argument, and finally unpacking the
  # TypedTable returned into a format readable by C++
  function runStage(args...)
    numDims = length(args) - 3
    ptrArray = args[length(args) - 2]
    userFn = args[length(args)]

    # Build a dictionary of dimName:dimValArray
    dims = Dict{Symbol,Array}()
    for i = 1:numDims
        dimName = Symbol(extractString(ptrArray, i, numDims))
        dims[dimName] = args[i]
    end

    # Convert to a TypedTable
    nt = NamedTuple{Tuple(keys(dims))}(values(dims))
    tbl = Table(nt)

    # Run the user-supplied function on the input data
    ret = userFn(tbl)

    # Convert the TypedTable back into a format that is readable from C++
    return unwrapRet(ret)
  end

  # Convert TypedTable into an array of arrays such that the final array is a list of dimension
  # names corresponding to the preceding arrays
  function unwrapRet(ret)
    result = []
    dims = []
    for colname in TypedTables.columnnames(ret)
      col = Base.getproperty(ret, colname)

      push!(dims, string(colname))
      push!(result, col)
    end

    # Add the dim names in order as the last element in the result array passed back to C++
    push!(result, dims)

    return result
  end

  # Use the list of pointers to string starts to extract a single string
  function extractString(ptrArray, i, numDims)
      if i == numDims
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ))
      else 
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ), ptrArray[i + 1] - ptrArray[i] )
      end
  end


end # module
