#
# a builtin class, subclasses of which support string invocation for methods
# (sort of)
# this is dependent upon Idol internals which are subject to change...
#
class strinvokable()
  method eval(s,args[])
    i := 1
    every methodname := name(!(self.__methods)) do {
	methodname[1 : find(".",methodname)+1 ] := ""
	if s == methodname then {
	    suspend self.__methods[i] ! ([self]|||args)
	    fail
	}
	i +:= 1
    }
  end
end
