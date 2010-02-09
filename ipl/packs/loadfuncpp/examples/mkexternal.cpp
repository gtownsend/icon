
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"
using namespace Icon;

extern "C" int iexample(int argc, value argv[]) {
	argv[0] = new external();
    return SUCCEEDED;
}

