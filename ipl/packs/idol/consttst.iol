const foo := 1
global barfoo
procedure baz()
  barfoo := "OK"
end
procedure main()
  baz()
  bar1 := "gag!"
  write(foo)
  write(barfoo)
  write("foo")
end
