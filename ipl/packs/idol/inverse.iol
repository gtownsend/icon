class inverse:fraction(d)
initially
  self.n := 1
end

procedure main()
  x := inverse(2)
  y := fraction(3,4)
  z := x$times(y)
  write("The decimal equivalent of ",z$asString(),
	" is ",trim(z$asReal(),'0'))
end
