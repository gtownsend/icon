/* test error message: token concatenation at end of macro */

#define f(a) a##
f(1)
