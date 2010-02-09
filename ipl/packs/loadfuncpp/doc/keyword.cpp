
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;

extern "C" int assignprog(value argv[]) {
    safe newname(argv[1]);
	&progname = newname;
	return FAILED;
}

