procedure main()
  decimal   := sequence(255)
  hex       := sequence("0123456789ABCDEF","0123456789ABCDEF")
  octal     := sequence(3,7,7)
  character := sequence(string(&cset))
  while write(right($@decimal,3)," ",$@hex," ",$@octal," ",image($@character))
end
