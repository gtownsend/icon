/* test recursion in multi-line macros */
#undef __RCRS__
#begdef f(x)
x
#if x >= 4
#undef f
#endif
f(x+1)
#enddef

f(1)

#begdef f(text, n)
#if n > 0
text
f(text, n-1)
#endif
#enddef

#define __RCRS__ 4
f(test of recursion, 3)

#undef __RCRS__
#define __RCRS__ 2
f(limited recursion, 4)
