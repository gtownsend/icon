/* test mutliple sequences of intervening white space */

"a"
#include "wh_sp1.h"
"c"

#undef __RCRS__
#begdef f(n)
#if n > 1



f(n-1)
#endif
"*"
#enddef
-">"f(6)
