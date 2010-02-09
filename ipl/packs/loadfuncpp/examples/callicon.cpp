
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make <platform>' to build.
 * For available <platform>s type 'make'.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"



extern "C" int iexample(int argc, value argv[]) {
	argv[0] = argv[1].apply(argv[2]);
    return SUCCEEDED;
}


