/*
 *  This is a dummy co-expression context switch that can be used in
 *  the absence of a working one.
 *
 *  There's problem with it under the Icon compiler, since the error occurs in
 *  the middle of the context switch. If the debugging flag is on, you get an
 *  empty file name and a 0 line number for the location. This is because the
 *  error appears to occur in the co-expression, but no code has yet been
 *  executed. If co-expressions are disabled with #define NoCoexpr, you
 *  won't get this far so it shouldn't be a problem.
 */  
#include "../h/rt.h"

int coswitch(old_cs, new_cs, fnc)
word *old_cs, *new_cs;
continuation fnc;
   {
   err_msg(401, NULL);
   }
