#
# Here is a sample test of automatic parenthesizing
#
class autotest(public yo)
  method foo(x)
    return x
  end
initially
  self.yo := "yo, bro"
end

procedure main()
  x := autotest()
  write(x$foo(x$yo)) # yo almost becomes a data item, notation-wise
end
