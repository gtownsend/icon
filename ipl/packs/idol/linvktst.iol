#
# List invocation for methods.  Icon uses binary ! but Idol
# uses $! for "foreach", so list invocation is specified via $$.
#

class abang()
  method a(args[])
    write("a:")
    every write (image(!args))
  end
end

class bbang : abang()
  method b(args[])
    write("b:")
    every write (image(!args))
    return self $$ a(["yo"]|||args)
  end
end

procedure main()
  x := bbang()
  x$b("yin","yang")

end
