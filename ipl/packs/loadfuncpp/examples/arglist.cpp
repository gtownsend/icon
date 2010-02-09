
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make <platform>' to build.
 * For available <platform>s type 'make'.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"


extern "C" int iexample(int argc, value argv[]) {
    safe x(argc, argv); //make the arguments into an Icon list
	argv[0] = x;
    return SUCCEEDED;
}


