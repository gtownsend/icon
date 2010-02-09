

/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */


#include "loadfuncpp.h"
using namespace Icon;

class myval: public external {
  public:
	virtual value name() { return "my external"; }
};

extern "C" int myext(value argv[]) {
	argv[0] = new myval();
	return SUCCEEDED;
}

extern "C" int ismine(value argv[]) {
	if( argv[1].isExternal("my external") )
		argv[0] = "Yes!";
	else
		argv[0] = "No!";
    return SUCCEEDED;
}


