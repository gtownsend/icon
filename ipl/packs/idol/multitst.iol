class multitst( a, b, c, d, e,
		f, g, h
		, i, j, k)
  method writemsg(x,y,z)
    write(x,y,z)
  end
  method write( plus,
		   other
		   ,stuff)
    every write(image(!self))
    write(plus,other,stuff)
  end
initially
  self$writemsg(
	     "this ",
	     "is ","not the")
  self$writemsg
    ("this is a","classical Icon-style bug","and it isn't printed")
  self$writemsg("this ",
	     "is ","almost the")
  self$writemsg()
  self$write("end","of","test")
end

procedure main()
  multitst("hi","there","this",,"is",1,"test")
end
