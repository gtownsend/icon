class sinvbuffer : strinvokable()
  method forward_char()
    write("success")
  end
  method eval(s,args[])
    suspend self$strinvokable.eval(map(s,"-","_"))
  end
end

procedure main()
  x := sinvbuffer()
  x $ eval("forward-char")
end
