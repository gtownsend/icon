class ictester()
  method classmethod()
    write("hello, world")
  end
end

procedure main()
  x := ictester()
  x$classmethod()
  ictester_classmethod(x)
end
