#
# Tests for the various builtins
#
procedure main()

  x := Table(1)
  write("\nTesting class ",x$class())
  write("Fields:")
  every write("\t", x$fieldnames )
  write("Methods:")
  every write("\t", x$methodnames )
  write()
  x$setElement("world","hello")
  write(x$getElement("world"))
  write(x$getElement("hello"))

  x := Deque()
  write("\nTesting class ",x$class())
  x$push("hello")
  x$push("world")
  write("My deque is size ",$*x)
  every write("give me a ",$!x)
  write("A random element is ",$?x)
  write("getting ",x$get()," popping ",x$pop())

  x := List(["Tucson", "Pima", 85721])
  write("\nTesting class ",x$class())
  every write("give me a ",$!x)

end
