class buffer(public filename,text,index)
  # read a buffer in from a file
  method read()
    f := open(self.filename,"r") | fail
    self$erase()
    every put(self.text,!f)
    close(f)
    return
  end
  # write a buffer out to a file
  method write()
    f := open(self.filename,"w") | fail
    every write(f,!self.text)
    close(f)
  end
  # insert a line at the current index
  method insert(s)
    if self.index = 1 then {
      push(self.text,s)
    } else if self.index > *self.text then {
      put(self.text,s)
    } else {
      self.text := self.text[1:self.index]|||[s]|||self.text[self.index:0]
    }
    self.index +:= 1
    return
  end
  # delete a line at the current index
  method delete()
    if self.index > *self.text then fail
    rv := self.text[self.index]
    if self.index=1 then pull(self.text)
    else if self.index = *self.text then pop(self.text)
    else self.text := self.text[1:self.index]|||self.text[self.index+1:0]
    return rv
  end
  # move the current index to an arbitrary line
  method goto(l)
    if (1 <= l) & (l <= *self.text+1) then return self.index := l
  end
  # return the current line and advance the current index
  method forward()
    if self.index > *self.text then fail
    rv := self.text[self.index]
    self.index +:= 1
    return rv
  end
  # place the buffer's text into a contiguously allocated list
  method linearize()
    tmp := list(*self.text)
    every i := 1 to *tmp do tmp[i] := self.text[i]
    self.text := tmp
  end
  method erase()
    self.text     := [ ]
    self.index    := 1
  end
  method size()
    return *(self.text)
  end
initially
  if \ (self.filename) then {
    if not self$read() then self$erase()
  } else {
    self.filename := "*scratch*"
    self.erase()
  }
end


class buftable : buffer()
  method read()
    self$buffer.read()
    tmp := table()
    every line := !self.text do
      line ? { tmp[tab(many(&ucase++&lcase))] := line | fail }
    self.text := tmp
    return
  end
  method lookup(s)
    return self.text[s]
  end
end


class bibliography : buftable()
end


class spellChecker : buftable(parentSpellChecker)
  method spell(s)
    return \ (self.text[s]) | (\ (self.parentSpellChecker))$spell(s)
  end
end


class dictentry(word,pos,etymology,definition)
  method decode(s) # decode a dictionary entry into its components
    s ? {
      self.word       := tab(upto(';'))
      move(1)
      self.pos        := tab(upto(';'))
      move(1)
      self.etymology  := tab(upto(';'))
      move(1)
      self.definition := tab(0)
    }
  end
  method encode()  # encode a dictionary entry into a string
    return self.word||";"||self.pos||";"||self.etymology||";"||self.definition
  end
initially
  if /self.pos then {
    # constructor was called with a single string argument
    self$decode(self.word)
  }
end

class dictionary : buftable()
  method read()
    self$buffer.read()
    tmp := table()
    every line := !self.text do
      line ? { tmp[tab(many(&ucase++&lcase))] := dictentry(line) | fail }
    self.text := tmp
  end
  method write()
    f := open(b.filename,"w") | fail
    every write(f,(!self.text)$encode())
    close(f)
  end
end
