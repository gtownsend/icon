
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make <platform>' to build.
 * For available <platform>s type 'make'.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"

extern "C" int activate(int argc, value argv[]) {
	argv[0] = argv[1].activate();
    return SUCCEEDED;
}

extern "C" int refresh(int argc, value argv[]) {
	argv[0] = argv[1].refreshed();
    return SUCCEEDED;
}

