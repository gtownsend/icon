class indextst()
  method index(y)
    write("index(",y,")")
  end
end

procedure main()
  x := indextst()
  x $[ "hello, world" ]
end
