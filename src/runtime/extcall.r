/*
 * extcall.r
 */

#if !COMPILER
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

#endif					/* ExternalFunctions */
#endif					/* !COMPILER */
