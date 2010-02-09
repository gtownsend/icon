
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;


struct addup: public iterate {
    safe total;
	addup(): total((long)0) {}
	virtual void takeNext(const value& x) {
		total = total + x;
	}
};

extern "C" int iexample(int argc, value argv[]) {
 	addup sum;
 	sum.every(argv[1], argv[2]);
 	argv[0] = sum.total;
    return SUCCEEDED;
}

