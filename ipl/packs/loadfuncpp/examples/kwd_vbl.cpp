
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"
using namespace Icon;

extern "C" int iexample(int argc, value argv[]) {
    safe y = argv[1];
	&progname = y;
	argv[0] = &progname;
    return SUCCEEDED;
}

