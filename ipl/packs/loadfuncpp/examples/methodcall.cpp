
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;

#include<cstdio>

extern "C" int iexample(int argc, value argv[]) {

 
    return SUCCEEDED;
}

