module PdalJulia

  using TypedTables

  function extractString(ptrArray, i, numDims)
      if i == numDims
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ))
      else 
          return unsafe_string( convert( Ptr{UInt8}, ptrArray[i] ), ptrArray[i + 1] - ptrArray[i] )
      end
  end

  function wrapArgs(args...)
    numDims = length(args) - 2
    ptrArray = args[length(args) - 1] # 2nd last arg is an array of pointers to the names of each dim

    # Build a dictionary of dimName:dimValArray
    dims = Dict{Symbol,Array}()
    for i = 1:numDims
        dimName = Symbol(extractString(ptrArray, i, numDims))
        dimValArray = args[i]
        dims[dimName] = dimValArray
    end

    # Convert the Dict into a named tuple
    nt = NamedTuple{Tuple(keys(dims))}(values(dims))

    # Finally return as a TypedTable
    return Table(nt)
  end

  function unwrapRet(args...)
    println(args)
    return args
  end

end # module
