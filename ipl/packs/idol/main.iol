procedure main()
  mydeque := Deque()
  mydeque$push("hello")
  mydeque$push("world")
  write("My deque is size ",mydeque$size())
  every write("give me a ",mydeque$foreach())
  write("A random element is ",mydeque$random())
  write("getting ",mydeque$get()," popping ",mydeque$pop())
end
