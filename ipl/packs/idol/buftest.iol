# buffer classes' tests

procedure main(args)
  if *args=0 then stop("usage: buftest cp file1 file2")
  every i := 1 to *args do {
      case args[i] of {
	  "cp": {
	      cp(args)
	  }
      }
  }
end
procedure cp(args)
  b1 := buffer(args[2])
  b2 := buffer(args[3])
  b2$erase()
  while s:=b1$forward() do b2$insert(s)
  b2$write()
end
