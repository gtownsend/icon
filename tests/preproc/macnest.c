/* test nesting of directives within multi-line macros */
#begdef foo(y,z)
   #undef x
   #begdef x
      #undef z##1
      #begdef z##1
         01
         02
         03
      #enddef
      y
   #enddef
#enddef
x
a1
b1
foo(1,a)
x
a1
b1
foo(2,b)
x
a1
b1
#begdef include(s)
#include #s
#enddef
include(macnest.h)

#begdef conditionals(a)
#if a
a_true
#else
a_false
#endif
#enddef
#define n 1
conditionals(n)
#undef n
#define n 0
conditionals(n)
