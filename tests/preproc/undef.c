/* test #undef */

#undef __RCRS__
#define x 3
#begdef f(a,b)
a+b
#enddef
#undef x
#undef f
__RCRS__
x
f(1,2)
