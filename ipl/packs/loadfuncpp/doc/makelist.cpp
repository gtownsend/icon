
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;

extern "C" int makelist(int argc, value argv[]) {
    safe arglist(argc, argv);
	argv[0] = arglist;
	return SUCCEEDED;
}

