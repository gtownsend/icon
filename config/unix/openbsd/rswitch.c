/*
 *  This is a dummy co-expression context switch that can be used in
 *  the absence of a working one.
 */  
#include "../h/rt.h"

int coswitch(old_cs, new_cs, fnc)
word *old_cs, *new_cs;
continuation fnc;
   {
   err_msg(401, NULL);
   }
