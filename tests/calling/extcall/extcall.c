#include "../h/config.h"
#include "../h/rt.h"
#include "rproto.h"

#ifdef ExternalFunctions

/*
 * extcall - stub procedure for external call interface.
 */
dptr extcall(dargv, argc, ip)
dptr dargv;
int argc;
int *ip;
   {
   *ip = 216;			/* no external function to find */
   return (dptr)NULL;
   }

#else					/* ExternalFunctions */
static char x;			/* prevent empty module */
#endif 					/* ExternalFunctions */
