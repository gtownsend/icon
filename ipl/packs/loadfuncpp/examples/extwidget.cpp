
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;

#include <cstdio>

class Widget: public external {
	long state;
  public:
  	Widget(long x): state(x) {}
  	
  	virtual value name() {
  		return "Widget";
  	}  	
  	virtual external* copy() {
  		return new Widget(state);
  	}
  	virtual value image() {
  		char sbuf[100];
  		sprintf(sbuf, "Widget_%ld(%ld)", id, state);
  		return value(NewString, sbuf);
  	}  	
};

extern "C" int iexample(int argc, value argv[]) {
	argv[0] = new Widget(99);
    return SUCCEEDED;
}

