module TestModule

  function runFilter(ins)
    for n in 1:length(ins)
      row = ins[n]
      row = merge(row, (; :X => 99.0, :Y => 999.0))
      ins[n] = row
    end
    return ins
  end

end # module
