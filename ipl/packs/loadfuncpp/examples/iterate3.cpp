
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;


struct addup: public iterate {
    safe total;
	int count;
	addup(): total((long)0) {
		count = 0;
	}
	virtual void takeNext(const value& x) {
		total = total + x;
	}
	virtual bool wantNext(const value& x) {
		return ++count <= 3;
	}
};

extern "C" int iexample(value argv[]) {
 	addup sum;
 	sum.bang(argv[1]);
 	argv[0] = sum.total;
    return SUCCEEDED;
}

