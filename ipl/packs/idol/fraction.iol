class fraction(n,d)
  method n()
    return self.n
  end
  method d()
    return self.d
  end
  method times(f)
    return fraction(self.n * f$n(), self.d * f$d())
  end
  method asString()
    return self.n||"/"||self.d
  end
  method asReal()
    return real(self.n) / self.d
  end
initially
  if self.d=0 then stop("fraction: denominator=0")
end
