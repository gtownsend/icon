
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

	addup(): total(0L), count(0) {}

	virtual void takeNext(const value& x) {
		total = total + x;
	}
	virtual bool wantNext(const value& x) {
		return ++count <= 10;
	}
};


extern "C" int sum10(value argv[]){
 	addup sum;
 	sum.every(argv[1], argv[2]);
 	argv[0] = sum.total;
    return SUCCEEDED;
}


