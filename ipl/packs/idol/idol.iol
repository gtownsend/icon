#
# global variables
#
global fin,fout,fName,fLine,alpha,alphadot,white,nonwhite,nonalpha
global classes,comp,exec,strict,links,imports,loud,compiles,compatible,ct
#
# gencode first generates specifications for all defined classes
# It then imports those classes' specifications which it needs to
# compute inheritance.  Finally, it writes out all classes' .icn files.
#
procedure gencode()
  if \loud then write("Class import/export:")
  #
  # export specifications for each class
  #
  every cl := classes$foreach_t() do cl$writespec()
  #
  # import class specifications, transitively
  #
  repeat {
    added := 0
    every super:= ((classes$foreach_t())$foreachsuper() | !imports) do{
      if /classes$lookup(super) then {
	added := 1
	fname := filename(super)
	readinput(envpath(fname),2)
	if /classes$lookup(super) then halt("can't import class '",super,"'")
	writesublink(fname)
      }
    }
    if added = 0 then break
  }
  #
  # compute the transitive closure of the superclass graph
  #
  every (classes$foreach_t())$transitive_closure()
  #
  # generate output
  #
  if \loud then write("Generating code:")
  writesublink("i_object")
  every s := !links do writelink(s)
  write(fout)
  every out := $!classes do {
    name := filename(out$name())
    out$write()
    put(compiles,name)
    writesublink(name)
  }
  if *compiles>0 then return cdicont(compiles)
  else return
end

#
# a class defining objects resulting from parsing lines of the form
# tag name ( field1 , field2, ... )
# If the constructor is given an argument, it is passed to self$read
#
class declaration(public name,fields,tag)
  #
  # parse a declaration string into its components
  #
  method read(decl)
    decl ? (
      (tab(many(white)) | "") ,
      # get my tag
      (self.tag := =("procedure"|"class"|"method"|"record")) ,
      (tab(many(white)) | "") ,
      # get my name
      (self.name := tab(many(alpha))) ,
      # get my fields
      (tab(find("(")+1)),
      (tab(many(white)) | "") ,
      ((self.fields := classFields())$parse(tab(find(")"))))
    ) | halt("declaration/read can't parse decl ",decl)
  end

  #
  # write a declaration; at the moment, only used by records
  #
  method write(f)
     write(f,self$String())
  end
  #
  # convert self to a string
  #
  method String()
    return self.tag || " " || self.name || "(" || self.fields$String() || ")"
  end
initially
  if \self.name then self$read(self.name)
end

#
# A class for ordinary Icon global declarations
#
class vardecl(s)
  method write(f)
    write(f,self.s)
  end
end

#
# A class defining the constants for a given scope
#
class constant(t)
  method expand(s)
    i := 1
    #
    # conditions for expanding a constant:
    # must not be within a larger identifier nor within a quote
    #
    while ((i <- find(k <- $!self,s,i)) & ((i=1) | any(nonalpha,s[i-1])) &
	  ((*s = i+*k-1) | any(nonalpha,s[i+*k])) &
          notquote(s[1:i])) do {
	val := \ (self.t[k]) | stop("internal error in expand")
	s[i +: *k] := val
#	i +:= *val
    }
    return s
  end
  method foreach() # in this case, we mean the keys, not the values
    suspend key(self.t)
  end
  method eval(s)
    if s2 := \ self.t[s] then return s2
  end
  method parse(s)
    s ? {
	k := trim(tab(find(":="))) | fail
	move(2)
	tab(many(white))
	val := tab(0) | fail
	(*val > 0) | fail
	self.t [ k ] := val
    }
    return
  end
  method append(cd)
    every s := cd$parse do self$parse(s)
  end
initially
  self.t := table()
end

#
# A class defining a single constant declaration
#
class constdcl : vardecl()
  # suspend the individual constant := value strings
  method parse()
    self.s ? {
	tab(find("const")+6)
	tab(many(white))
	while s2 := trim(tab(find(","))) do {
	    suspend s2
	    move(1)
	    tab(many(white))
	}
	suspend trim(tab(0))
    }
  end
end

#
# class body manages a list of strings holding the code for
# procedures/methods/classes
#
class body(fn,ln,vars,text)
  method read()
    self.fn    := fName
    self.ln    := fLine
    self.text  := []
    while line := readln() do {
      put(self.text, line)
      line ? {
	  tab(many(white))
	  if ="end" & &pos > *line then return
	  else if =("local"|"static"|"initial") & any(nonalpha) then {
	      self.ln +:= 1
	      pull(self.text)
	      / (self.vars) := []
	      put(self.vars, line)
	  }
      }
    }
    halt("body/read: eof inside a procedure/method definition")
  end
  method write(f)
    if \self.vars then every write(f,!self.vars)
    if \compatible then write(f,"  \\self := self.__state")
    if \self.ln then
	write(f,"#line ",self.ln + ((*\self.vars)|0)," \"",self.fn,"\"")
    every write(f,$!self)
  end
  method delete()
    return pull(self.text)
  end
  method size()
    return (*\ (self.text)) | 0
  end
  method foreach()
    if t := \self.text then suspend !self.text
  end
end

#
# a class defining operations on classes
#
class class : declaration (supers,methods,text,imethods,ifields,glob)
  # imethods and ifields are all lists of these:
  record classident(class,ident)

  method read(line,phase)
    self$declaration.read(line)
    self.supers := idTaque(":")
    self.supers$parse(line[find(":",line)+1:find("(",line)] | "")
    self.methods:= taque()
    self.text   := body()
    while line  := readln("wrap") do {
      line ? {
	tab(many(white))
	if ="initially" then {
	    self.text$read()
	    if phase=2 then return
	    self.text$delete()	# "end" appended manually during writing after
				# generation of the appropriate return value
	    return
	} else if ="method" then {
	    decl := method(self.name)
	    decl$read(line,phase)
	    self.methods$insert(decl,decl$name())
	} else if ="end" then {
	    # "end" is tossed here. see "initially" above
	    return
	} else if ="procedure" then {
	    decl := method("")
	    decl$read(line,phase)
	    /self.glob := []
	    put(self.glob,decl)
	} else if ="global" then {
	    /self.glob := []
	    put(self.glob,vardecl(line))
	} else if ="record" then {
	    /self.glob := []
	    put(self.glob,declaration(line))
	} else if upto(nonwhite) then {
	    halt("class/read expected declaration on: ",line)
	}
      }
    }
    halt("class/read syntax error: eof inside a class definition")
  end

  #
  # Miscellaneous methods on classes
  #
  method has_initially()
    return $*self.text > 0
  end
  method ispublic(fieldname)
    if self.fields$ispublic(fieldname) then return fieldname
  end
  method foreachmethod()
    suspend $!self.methods
  end
  method foreachsuper()
    suspend $!self.supers
  end
  method foreachfield()
    suspend $!self.fields
  end
  method isvarg(s)
    if self.fields$isvarg(s) then return s
  end
  method transitive_closure()
    count := $*self.supers
    while count > 0 do {
	added := taque()
	every sc := $!self.supers do {
	  if /(super := classes$lookup(sc)) then
	    halt("class/transitive_closure: couldn't find superclass ",sc)
	  every supersuper := super$foreachsuper() do {
	    if / self.supers$lookup(supersuper) &
		 /added$lookup(supersuper) then {
	      added$insert(supersuper)
	    }
	  }
	}
	count := $*added
	every self.supers$insert($!added)
    }
  end
  #
  # write the class declaration: if s is "class" write as a spec
  # otherwise, write as a constructor
  #
  method writedecl(f,s)
    writes(f, s," ",self.name)
    if s=="class" & ( *(supers := self.supers$String()) > 0 ) then
	    writes(f," : ",supers)
    writes(f,"(")
    rv := self.fields$String(s)
    if *rv > 0 then rv ||:= ","
    if s~=="class" & *(\self.ifields)>0 then	{	# inherited fields
      every l := !self.ifields do rv ||:= l.ident || ","
      if /(superclass := classes$lookup(l.class)) then
	  halt("class/resolve: couldn't find superclass ",sc)
      if superclass$isvarg(l.ident) then rv := rv[1:-1]||"[],"
    }
    writes(f,rv[1:-1])
    write(f,,")")
  end
  method writespec(f) # write the specification of a class
    f := envopen(filename(self.name),"w")
    self$writedecl(f,"class")
    every ($!self.methods)$writedecl(f,"method")
    if self$has_initially() then write(f,"initially")
    write(f,"end")
    close(f)
  end

  #
  # write out the Icon code for this class' explicit methods
  # and its "nested global" declarations (procedures, records, etc.)
  #
  method writemethods()
    f:= envopen(filename(self.name,".icn"),"w")
    every ($!self.methods)$write(f,self.name)

    if \self.glob & *self.glob>0 then {
	write(f,"#\n# globals declared within the class\n#")
	every i := 1 to *self.glob do (self.glob[i])$write(f,"")
    }
    close(f)
  end

  #
  # write - write an Icon implementation of a class to file f
  #
  method write()
    f:= envopen(filename(self.name,".icn"),"a")
    #
    # must have done inheritance computation to write things out
    #
    if /self.ifields then self$resolve()

    #
    # write a record containing the state variables
    #
    writes(f,"record ",self.name,"__state(__state,__methods") # reserved fields
    rv := ","
    rv ||:= self.fields$idTaque.String()		     # my fields
    if rv[-1] ~== "," then rv ||:= ","
    every s := (!self.ifields).ident do rv ||:= s || "," # inherited fields
    write(f,rv[1:-1],")")

    #
    # write a record containing the methods
    #
    writes(f,"record ",self.name,"__methods(")
    rv := ""

    every s := ((($!self.methods)$name())	|	# my explicit methods
		self.fields$foreachpublic()	|	# my implicit methods
		(!self.imethods).ident		|	# my inherited methods
		$!self.supers)				# super.method fields
	do rv ||:= s || ","

    if *rv>0 then rv[-1] := ""			# trim trailling ,
    write(f,rv,")")

    #
    # write a global containing this classes' operation record
    # along with declarations for all superclasses op records
    #
    writes(f,"global ",self.name,"__oprec")
    every writes(f,", ", $!self.supers,"__oprec")
    write(f)

    #
    # write the constructor procedure.
    # This is a long involved process starting with writing the declaration.
    #
    self$writedecl(f,"procedure")
    write(f,"local self,clone")

    #
    # initialize operation records for this and superclasses
    #
    write(f,"initial {\n",
	    "  if /",self.name,"__oprec then ",self.name,"initialize()")
    if $*self.supers > 0 then
	every (super <- $!self.supers) ~== self.name do
	    write(f,"  if /",super,"__oprec then ",super,"initialize()\n",
		    "  ",self.name,"__oprec.",super," := ", super,"__oprec")
    write(f,"  }")

    #
    # create self, initialize from constructor parameters
    #
    writes(f,"  self := ",self.name,"__state(&null,",self.name,"__oprec")
    every writes(f,",",$!self.fields)
    if \self.ifields then every writes(f,",",(!self.ifields).ident)
    write(f,")\n  self.__state := self")

    #
    # call my own initially section, if any
    #
    if $*self.text > 0 then write(f,"  ",self.name,"initially(self)")

    #
    # call superclasses' initially sections
    #
    if $*self.supers > 0 then {
	every (super <- $!self.supers) ~== self.name do {
	    if (classes$lookup(super))$has_initially() then {
		if /madeclone := 1 then {
		    write(f,"  clone := ",self.name,"__state()\n",
			"  clone.__state := clone\n",
			"  clone.__methods := ",self.name,"__oprec")
		}
		write(f,"  # inherited initialization from class ",super)
		write(f,"    every i := 2 to *self do clone[i] := self[i]\n",
			"    ",super,"initially(clone)")
		every l := !self.ifields do {
		    if l.class == super then
			write(f,"    self.",l.ident," := clone.",l.ident)
		}
	    }
	}
    }

    #
    # return the pair that comprises the object:
    # a pointer to the instance (__mystate), and
    # a pointer to the class operation record
    #
    write(f,"  return idol_object(self,",self.name,"__oprec)\n",
	    "end\n")
    
    #
    # write out class initializer procedure to initialize my operation record
    #
    write(f,"procedure ",self.name,"initialize()")
    writes(f,"  initial ",self.name,"__oprec := ",self.name,"__methods")
    rv := "("
    every s := ($!self.methods)$name() do {		# explicit methods
      if *rv>1 then rv ||:= ","
      rv ||:= self.name||"_"||s
    }
    every me := self.fields$foreachpublic() do {	# implicit methods
      if *rv>1 then rv ||:= ","			# (for public fields)
      rv ||:= self.name||"_"||me
    }
    every l := !self.imethods do {			# inherited methods
      if *rv>1 then rv ||:= ","
      rv ||:= l.class||"_"||l.ident
    }
    write(f,rv,")\n","end")
    #
    # write out initially procedure, if any
    #
    if self$has_initially() then {
	write(f,"procedure ",self.name,"initially(self)")
	self.text$write(f)
	write(f,"end")
    }

    #
    # write out implicit methods for public fields
    #
    every me := self.fields$foreachpublic() do {
      write(f,"procedure ",self.name,"_",me,"(self)")
      if \strict then {
	write(f,"  if type(self.",me,") == ",
		"(\"list\"|\"table\"|\"set\"|\"record\") then\n",
		"    runerr(501,\"idol: scalar type expected\")")
	}
      write(f,"  return .(self.",me,")")
      write(f,"end")
      write(f)
    }

    close(f)

  end

  #
  # resolve -- primary inheritance resolution utility
  #
  method resolve()
    #
    # these are lists of [class , ident] records
    #
    self.imethods := []
    self.ifields := []
    ipublics := []
    addedfields := table()
    addedmethods := table()
    every sc := $!self.supers do {
	if /(superclass := classes$lookup(sc)) then
	    halt("class/resolve: couldn't find superclass ",sc)
	every superclassfield := superclass$foreachfield() do {
	    if /self.fields$lookup(superclassfield) &
	       /addedfields[superclassfield] then {
		addedfields[superclassfield] := superclassfield
		put ( self.ifields , classident(sc,superclassfield) )
		if superclass$ispublic(superclassfield) then
		    put( ipublics, classident(sc,superclassfield) )
	    } else if \strict then {
		warn("class/resolve: '",sc,"' field '",superclassfield,
		     "' is redeclared in subclass ",self.name)
	    }
	}
	every superclassmethod := (superclass$foreachmethod())$name() do {
	    if /self.methods$lookup(superclassmethod) &
	       /addedmethods[superclassmethod] then {
		addedmethods[superclassmethod] := superclassmethod
		put ( self.imethods, classident(sc,superclassmethod) )
	    }
	}
	every public := (!ipublics) do {
	    if public.class == sc then
		put (self.imethods, classident(sc,public.ident))
	}
    }
  end
end

#
# a class defining operations on methods and procedures
#
class method : declaration (class,text)
  method read(line,phase)
    self$declaration.read(line)
    self.text := body()
    if phase = 1 then
      self.text$read()
  end
  method writedecl(f,s)
    decl := self$String()
    if s == "method" then decl[1:upto(white,decl)] := "method"
    else {
	decl[1:upto(white,decl)] := "procedure"
	if *(self.class)>0 then {
	    decl[upto(white,decl)] ||:= self.class||"_"
	    i := find("(",decl)
	    decl[i] ||:= "self" || (((decl[i+1] ~== ")"), ",") | "")
	}
    }
    write(f,decl)
  end
  method write(f)
    if self.name ~== "initially" then
	self$writedecl(f,"procedure")
    self.text$write(f)
    self.text := &null			# after writing out text, forget it!
  end
end

#
# a class corresponding to an Icon table, with special treatment of empties
#
class Table(t)
  method size()
    return (* \ self.t) | 0
  end
  method insert(x,key)
    /self.t := table()
    /key := x
    if / (self.t[key]) := x then return
  end
  method lookup(key)
    if t := \self.t then return t[key]
    return
  end
  method foreach()
    if t := \self.t then every suspend !self.t
  end
end

#
# tabular queues (taques):
# a class defining objects which maintain synchronized list and table reps
# Well, what is really provided are loosely-coordinated list/tables
#
class taque : Table (l)
  method insert(x,key)
    /self.l := []
    if self$Table.insert(x,key) then put(self.l,x)
  end
  method foreach()
    if l := \self.l then every suspend !self.l
  end
  method insert_t(x,key)
    self$Table.insert(x,key)
  end
  method foreach_t()
    suspend self$Table.foreach()
  end
end

#
# support for taques found as lists of ids separated by punctuation
# constructor called with (separation char, source string)
#
class idTaque : taque(punc)
  method parse(s)
    s ? {
      tab(many(white))
      while name := tab(find(self.punc)) do {
	self$insert(trim(name))
	move(1)
	tab(many(white))
      }
      if any(nonwhite) then self$insert(trim(tab(0)))
    }
    return
  end
  method String()
    if /self.l then return ""
    out := ""
    every id := !self.l do out ||:= id||self.punc
    return out[1:-1]
  end
end

#
# parameter lists in which the final argument may have a trailing []
#
class argList : idTaque(public varg)
  method insert(s)
    if \self.varg then halt("variable arg must be final")
    if i := find("[",s) then {
      if not (j := find("]",s)) then halt("variable arg expected ]")
      s[i : j+1] := ""
      self.varg := s := trim(s)
    }
    self$idTaque.insert(s)
  end
  method isvarg(s)
    if s == \self.varg then return s
  end
  method String()
    return self$idTaque.String() || ((\self.varg & "[]") | "")
  end
initially
  self.punc := ","
end

#
# Idol class field lists in which fields may be preceded by a "public" keyword
#
class classFields : argList(publics)
  method String(s)
    if *(rv := self$argList.String()) = 0 then return ""
    if /s | (s ~== "class") then return rv
    if self$ispublic(self.l[1]) then rv := "public "||rv
    every field:=self$foreachpublic() do rv[find(","||field,rv)] ||:= "public "
    return rv
  end
  method foreachpublic()
    if \self.publics then every suspend !self.publics
  end
  method ispublic(s)
    if \self.publics then every suspend !self.publics == s
  end
  method insert(s)
    s ? {
      if ="public" & tab(many(white)) then {
	s := tab(0)
	/self.publics := []
	put(self.publics,s)
      }
    }
    self$argList.insert(s)
  end
initially
  self.punc := ","
end

#
# procedure to read a single Idol source file
#
procedure readinput(name,phase,ct2)
    if \loud then write("\t",name)
    fName := name
    fLine := 0
    fin   := sysopen(name,"r")
    ct    := \ct2 | constant()
    while line := readln("wrap") do {
	line ? {
	    tab(many(white))
	    if ="class" then {
		decl := class()
		decl$read(line,phase)
		if phase=1 then {
		    decl$writemethods()
		    classes$insert(decl,decl$name())
		} else classes$insert_t(decl,decl$name())
	    }
	    else if ="procedure" then {
		if comp = 0 then comp := 1
		decl := method("")
		decl$read(line,phase)
		decl$write(fout,"")
		}
	    else if ="record" then {
		if comp = 0 then comp := 1
		decl := declaration(line)
		decl$write(fout,"")
		}
	    else if ="global" then {
		if comp = 0 then comp := 1
		decl := vardecl(line)
		decl$write(fout,"")
		}
	    else if ="const" then {
		ct$append ( constdcl(line) )
	        }
	    else if ="method" then {
		halt("readinput: method outside class")
	        }
	    else if ="#include" then {
		savedFName := fName
		savedFLine := fLine
		savedFIn   := fin
		tab(many(white))
		readinput(tab(if ="\"" then find("\"") else many(nonwhite)),
			  phase,ct)
		fName := savedFName
		fLine := savedFLine
		fin   := savedFIn
	        }
	}
    }
    close(fin)
end

#
# filter the input translating $ references
# (also eats comments and trims lines)
#
procedure readln(wrap)
  count := 0
  prefix := ""
  while /finished do {

    if not (line := read(fin)) then fail
    fLine +:= 1
    if match("#include",line) then return line
    line[ 1(x<-find("#",line),notquote(line[1:x])) : 0] := ""
    line := trim(line,white)
#    line := selfdot(line)
    x := 1
    while ((x := find("$",line,x)) & notquote(line[1:x])) do {
      z := line[x+1:0] ||" "	     # " " is for bal()
      case line[x+1] of {
        #
        # IBM 370 digraphs
        #
        "(": line[x+:2] := "{"
        ")": line[x+:2] := "}"
        "<": line[x+:2] := "["
        ">": line[x+:2] := "]"
        #
        # Invocation operators $! $* $@ $? (for $$ see below)
        #
        "!"|"*"|"@"|"?": {
          z ? {
	    move(1)
	    tab(many(white))
	    if not (id := tab(many(alphadot))) then {
	      if not match("(") then halt("readln can't parse ",line)
	      if not (id := tab(&pos<bal())) then
		  halt("readln: cant bal ",&subject)
	    }
	    Op := case line[x+1] of {
		"@": "activate"
		"*": "size"
		"!": "foreach"
		"?": "random"
	    }
	    count +:= 1
	    line[x:0] :=
		"(__self"||count||" := "||id||").__methods."||
		Op||"(__self"||count||".__state)"||tab(0)
	  }
        }
	#
	# x $[ y ] shorthand for x$index(y)
	#
	"[": {
	    z ? {
		if not (middle := tab((&pos<bal(&cset,'[',']'))-1)[2:0]) then
		    halt("readln: can't bal([) ",&subject)
		tail := tab(0)|""
		line := line[1:x]||"$index("||middle||")"||(tab(0)|"")
	    }
	}
        default: {
	    #
	    # get the invoking object.
	    #
	    reverse(line[1:x])||" " ? {
		tab(many(white))
		if not (id := reverse(tab(many(alphadot)))) then {
		    if not match(")") then halt("readln: can't parse")
		    if not (id := reverse(tab(&pos<bal(&cset,')','('))))
		    then halt("readln: can't bal ",&subject)
		}
		objlen := &pos-1
	    }
	    count +:= 1
	    front := "(__self"||count||" := "||id||").__methods."
	    back := "__self"||count||".__state"

	    #
	    # get the method name
	    #
	    z ? {
		="$"
		tab(many(white))
		if not (methodname := tab(many(alphadot))) then
		    halt("readln: expected a method name after $")
		tab(many(white))
		methodname ||:= "("
		if ="(" then {
		    tab(many(white))
		    afterlp := &subject[&pos]
		}
		else {
		    afterlp := ")"
		    back ||:= ")"
		}
		methlen := &pos-1
	    }
	    if line[x+1] == "$" then {
		c := if afterlp[1] ~== ")" then "" else "[]"
		methodname[-1] := "!("
		back := "["||back||"]|||"
	    } else {
		c := if (\afterlp)[1] == ")" then "" else ","
	    }
	    line[x-objlen : (((*line>=(x+methlen+1))|0)\1)] :=
		front || methodname || back || c
	}
      } # case
    } # while there's a $ to process
    if /wrap | (prefix==line=="") then finished := line
    else {
	prefix ||:= line || " "			# " " is for bal()
	prefix ? {
	    # we are done if the line is balanced wrt parens and
	    # doesn't end in a continuation character (currently just ,)
	    if ((*prefix = bal()) & (not find(",",prefix[-2]))) then
		finished := prefix[1:-1]
	}
    }
  } # while / finished
  return ct$expand(finished)
end
