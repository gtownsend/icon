# %W% %G%
#
# Builtin Icon objects, roughly corresponding to the language builtins.
# (These are not builtin to the Idol interpreter!)
#
# Taxonomy of builtin types:
#
#			       __Object___
#                           _-'           `-_
#			 _-' 	             `-_
#	      Collection			Atom_
#	     /    |    \                      _'     `-.
#	Stack	Queue	Vector		   _-'		Number
#           \   /      /  |  \          _-'            /      \ 
#	    Deque     /	  |   \	      _' 	Integer	       Real
#               \    /    |    \     /
#		List	Table	String
#
#	

#
# this is the Smalltalk-style ideal root of an inheritance hierarchy.
# add your favorite methods here.
#
class Object()
  # return the class name as a string
  method class()
    return image(self)[8:find("_",image(self))]
  end
  # generate the field names as strings
  method fieldnames()
    i := 1
    every s := name(!(self.__state)) do {
	if i>2 then s ? { tab(find(".")+1); suspend tab(0) }
	i +:= 1
    }
  end
  # generate the method names as strings
  method methodnames()
    every s := name(!(self.__methods)) do {
	s ? { tab(find(".")+1); suspend tab(0) }
    }
  end
end

# Collections support Icon's *?! operators
class Collection : Object (theCollection)
  method size()
    return *self.theCollection
  end
  method foreach()
    suspend !self.theCollection
  end
  method random()
    return ?self.theCollection
  end
end

# Vectors have the ability to access individual elements
class Vector : Collection()
  method getElement(i)
    return self.theCollection[i]
  end
  method setElement(i,v)
    return self.theCollection[i] := v
  end
end

class Table : Vector(initialvalue,theCollection)
initially
  self.theCollection := table(self.initialvalue)
end

#
# The field theCollection is explicitly named so that subclasses of Stack
# and Queue use these automatic initializations.  The / operator is used
# to reduce the number of throw-away list allocations for subclasses which
# >don't< inherit theCollection from Stack or Queue (e.g. class List).
# It also allows initialization by constructor.  If one wanted to
# guarantee that all Stacks start out empty but still allow class List
# to be explicitly intitialized, one could remove the / here, and name
# theCollection in class List, causing its initially section to override
# the superclass with respect to the field theCollection.  I choose here
# to maximize code sharing rather than protecting my Stack class.
#
# When allowing initialization by constructor one might consider
# checking the type of the input to guarantee it conforms to the
# type expected by the class.
#
class Stack : Collection(theCollection)
  method push(value)
    push(self.theCollection,value)
  end
  method pop()
    return pop(self.theCollection)
  end
initially
  /self.theCollection := []
end

class Queue : Collection(theCollection)
  method get()
    return get(self.theCollection)
  end
  method put(value)
    put(self.theCollection,value)
  end
initially
  /self.theCollection := []
end

# Deques are a first example of multiple inheritance.
class Deque : Queue : Stack()
end

#
# List inherits Queue's theCollection initialization, because Queue is the
# first class on List's (transitively closed) superclass list to name
# theCollection explicitly
#
class List : Deque : Vector()
  method concat(l)
    return List(self.theCollection ||| l)
  end
end

class Atom : Object(public val)
  method asString()
    return string(self.val)
  end
  method asInteger()
    return integer(self.val)
  end
  method asReal()
    return real(self.val)
  end
end

class Number : Atom ()
  method plus(n)
    return self.val + n$val()
  end
  method minus(n)
    return self.val - n$val()
  end
  method times(n)
    return self.val * n$val()
  end
  method divide(n)
    return self.val / n$val()
  end
end

class Integer : Number()
initially
  if not (self.val := integer(self.val)) then
    stop("can't make Integer from ",image(self.val))
end

class Real : Number()
initially
  if not (self.val := real(self.val)) then
    stop("can't make Real from ",image(self.val))
end

class String : Vector : Atom()
  method concat(s)
    return self.theCollection || s
  end
end
