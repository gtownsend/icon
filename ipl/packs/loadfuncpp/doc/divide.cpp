

/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */


#include "loadfuncpp.h"
using namespace Icon;

extern "C" int div(value argv[]) {
    safe x(argv[1]), y(argv[2]), z;
	z = ( x/y, x%y );
	argv[0] = z;
    return SUCCEEDED;
}


