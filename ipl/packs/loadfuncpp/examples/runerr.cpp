
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"

#include<stdio.h>

extern "C" int iexample(value argv[]) {
    safe callme(argv[1]), text(argv[2]);
	printf("Calling callme\n");
	callme();	
	printf("Callme returned\n");
	printf("Calling callme\n");
	callme();
	printf("Callme returned\n");
	//Icon::runerr(123, text);
    return FAILED;
}

extern "C" int iexample2(value argv[]) {
	//Icon::display(&Icon::level, &Icon::output);
	safe nextcall(argv[1]), rerr(argv[2]);
	nextcall();
	rerr(123, "Bye!");
	//Icon::runerr(123, "Bye!");
	return FAILED;
}
