procedure sequence(bounds[ ])
  return Sequence(bounds)
end

class Sequence(bounds,indices)
  method max(i)
    elem := self.bounds[i]
    return (type(elem)== "integer",elem) | *elem-1
  end
  method elem(i)
    elem := self.bounds[i]
    return (type(elem)== "integer",self.indices[i]) | elem[self.indices[i]+1]
  end
  method activate()
    top := *(self.indices)
    if self.indices[1] > self$max(1) then fail
    s := ""
    every i := 1 to top do {
      s ||:= self$elem(i)
    }
    repeat {
       self.indices[top] +:= 1
       if top=1 | (self.indices[top] <= self$max(top)) then break
       self.indices[top] := 0
       top -:= 1
    }
    return s
  end
initially
  / (self.indices) := list(*self.bounds,0)
end
