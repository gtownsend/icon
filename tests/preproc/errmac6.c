/* test error message: stringizing operator at end of macro */

#define f(a) a#
f(1)
