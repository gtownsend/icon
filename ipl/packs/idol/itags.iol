# itags - an Icon/Idol tag generator by Nick Kline
# hacks (such as this header comment) by Clint Jeffery
# last edit: 12/13/89
#
# the output is a sorted list of lines of the form
# identifier  owning_scope  category_type  filename  lineno(:length)
#
# owning scope is the name of the class or procedure or record in which
# the tag is defined.
# category type is the kind of tag; one of:
# (global,procedure,record,class,method,param,obj_field,rec_field)
#
global ibrowseflag

procedure main(args) 
local line, lineno, fout, i, fin, notvar, objects, actual_file, outlines

initial {
    fout := open("ITAGS", "w") | stop("can't open ITAGS for writing"); 
    outlines := [[0,0,0,0,0,0]]
    i := 1
    notid := &cset -- &ucase -- &digits -- &lcase -- '_'
}

if(*args=0) then 
    stop("usage: itags file1 [file2 ...]")

while i <= *args do {
    if args[i] == "-i" then {
	ibrowseflag := 1
	i +:= 1
	continue
    }
    fin := open(args[i],"r") |
	stop("could not open file ",args[i]," exiting")
    lineno := 1
    objects := program( args[i] )

    while line := read(fin) do {
	line[upto('#',line):0] := ""
	line ? {
	    tab(many(' ')) 
	    
	    if =("global") then {
		if(any(notid)) then 
		    every objects$addvar( getword(), lineno )
	    }
		
	    if =("procedure")  then 
		  if(any(notid)) then {
		    objects$addproc( getword(), lineno )
		    objects$myline(tab(0),lineno)
	    }
	    

	    if =("class") then 
		if any(notid) then {
		    objects$addclass( getword(), lineno )
		    objects$myline(tab(0),lineno)
		}


	    if =("method") then {
		if any(notid) then {
		    objects$addmethod( getword(), lineno ) 
		    objects$myline(tab(0),lineno)
		}
	    }

	    if =("local") then {
		if any(notid) then 
		    every objects$addvar( getword(), lineno ) 
	    }

	    if =("static") then {
		if any(notid) then 
		    every objects$addstat( getword(), lineno ) 
	    }

	    if =("record") then {
		if any(notid) then {
		    objects$addrec( getword(), lineno ) 
		    objects$myline(tab(0),lineno)
		    objects$endline( lineno)
		}
	    }
	    if =("end") then
		objects$endline(lineno)
	}
	lineno +:= 1
    }
    objects$drawthyself(outlines)
    i +:= 1
}
# now process all the resulting lines
every i := 2 to *outlines do {
    outlines[i] := (
	left(outlines[i][1],outlines[1][1]+1) ||
	left(outlines[i][2],outlines[1][2]+1) ||
	left(outlines[i][3],outlines[1][3]+1) ||
	left(outlines[i][4],outlines[1][4]+1) ||
	left(outlines[i][5],outlines[1][5]) ||
	(if \outlines[i][6] then ":"||outlines[i][6] else ""))
}
outlines := outlines[2:0]
outlines := sort(outlines)
every write(fout,!outlines)
end

class functions(name, lineno,vars,lastline, parent, params,stat,paramtype)

method drawthyself(outfile)
local k
    every k := !self.vars do
      emit(outfile, k[1], self.name, "local", self.parent$myfile(),k[2])
    every k := !self.params do
      emit(outfile, k[1], self.name, self.paramtype, self.parent$myfile(),k[2])
    every k := !self.stat do
      emit(outfile, k[1], self.name, "static", self.parent$myfile(),k[2])
end

method myline(line,lineno)
local word
static ids,  letters
initial {
    ids := &lcase ++ &ucase ++ &digits ++ '_'
    letters := &ucase ++ &lcase
}

line ? while tab(upto(letters)) do  {
    word := tab(many(ids))
    self.params|||:= [[word,lineno]]
}

end

method addstat(varname, lineno)
    self.stat|||:=[[varname, lineno]]
    return
end

method addvar(varname, lineno)
    self.vars|||:=[[varname, lineno]]
    return
end

method endline( lineno )
   self.lastline := lineno
end

method resetcontext()
    self.parent$resetcontext()
end

initially
    self.vars := []
    self.params := []
    self.stat := []
    self.paramtype := "param"
end # end of class functions


class proc : functions(name,lineno, parent,paramtype)

method drawthyself(outfile)
    emit(outfile,self.name, "*" , "procedure", self.parent$myfile(),self.lineno, self.lastline-self.lineno+1)
    self$functions.drawthyself(outfile)
end
initially
 self.paramtype := "param"
end # of class proc

class rec : functions(name, lineno, parent, line, paramtype)

method drawthyself(outfile)
    emit(outfile,self.name, "*", "record", self.parent$myfile(),
	 self.lineno)
    self$functions.drawthyself(outfile)
end
initially
  self.paramtype := "rec_field"
end # class record



class program(public myfile, vars, proc, records, classes, curcontext, contextsave,globals)

method endline( lineno )
    self.curcontext$endline( lineno )
    self.curcontext := pop(self.contextsave)
end

method myline( line,lineno)
    self.curcontext$myline( line,lineno)
end
   
method drawthyself(outfile)
    every k := !self.globals do
	emit(outfile,k[1], "*", "global", self.myfile,k[2])
    every (!self.proc)$drawthyself(outfile)
    every (!self.records)$drawthyself(outfile)
    every (!self.classes)$drawthyself(outfile)
end

method addmethod(name, lineno)
    push(self.contextsave,self.curcontext)
    self.curcontext := self.curcontext$addmethod(name,lineno)
    return
end

method addstat(varname, lineno)
    self.curcontext$addstat(varname, lineno)
end

method addvar(varname, lineno)
    if self.curcontext === self
    then  self.globals|||:= [[varname,lineno]]
    else self.curcontext$addvar(varname,lineno)
    return
end

method addproc(procname, lineno)
    push(self.contextsave, self.curcontext)
    self.curcontext := proc(procname, lineno, self)
    self.proc|||:= [self.curcontext]
    return
end

method addrec(recname, lineno)
    push(self.contextsave, self.curcontext)
    self.curcontext := rec(recname, lineno,self)
    self.records|||:=[self.curcontext]
    return
end

method addclass(classname, lineno)
    push(self.contextsave, self.curcontext)
    self.curcontext := class_(classname, lineno, self)
    self.classes|||:=[self.curcontext]
    return
end

method resetcontext()
    self.curcontext := pop(self.contextsave)
end

initially 
 self.globals := []
 self.proc := []
 self.records := []
 self.classes := []
 self.curcontext := self
 self.contextsave := []
end  # end of class program



class class_ : functions (public name, lineno, parent, meth,paramtype)

method myfile()
    return self.parent$myfile()
end

method addmethod(methname, lineno)
    self.meth|||:= [methods(methname, lineno, self)]
    return (self.meth[-1])
end

method drawthyself(outfile)
    emit(outfile,self.name, "*" , "class", self.parent$myfile(),self.lineno, self.lastline-self.lineno+1)    
    every (!self.meth)$drawthyself(outfile)
    self$functions.drawthyself(outfile)
end

initially
    self.meth := []
    self.paramtype := "obj_field"
end #end of class_

class methods: functions(name, lineno, parent,paramtype)
method drawthyself(outfile)
        emit(outfile,self.name, self.parent$name() , "method", self.parent$myfile(),self.lineno, self.lastline-self.lineno+1)    
    self$functions.drawthyself(outfile)
end
initially
    self.paramtype := "param"
end #end of members    class

procedure emit(outlist,ident, scope, type, filename, line, length)
    outlist[1][1] := outlist[1][1] < *ident
    outlist[1][2] := outlist[1][2] < *scope
    outlist[1][3] := outlist[1][3] < *type
    outlist[1][4] := outlist[1][4] < *filename
    outlist[1][5] := outlist[1][5] < *line
    outlist[1][6] := outlist[1][6] < *\length
    if /ibrowseflag then
       put( outlist, [ident,scope,type,filename,line,length] )
    else
       put( outlist, [ident,scope,type,filename,line,length] )
end


procedure getword()
    local word
    static ids,letts
    initial {
	ids := &ucase ++ &lcase ++ &digits ++ '_'
	letts := &ucase ++ &lcase
    }

    while tab(upto(letts)) do {
	word := tab(many(ids))
	suspend word
    }

end
