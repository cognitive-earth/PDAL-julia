module TestModule

  function fff(ins)
    for n in 1:length(ins)
      row = ins[n]
      row = merge(row, (; :X => 99.0))
      row = merge(row, (; :Y => 999.0))
      ins[n] = row
    end
    return ins
  end

end # module
