#define m 1 2 3
#define m 1	2     3
m

#define f(a,b) a      b
#define f(   a   ,   b   ) a b
f(1,2)

#begdef r(a)
a
#enddef
#begdef r(a)
a

#enddef
r("hi")
