/*
 *  This is a dummy co-expression context switch that can be used in
 *  the absence of a working one.
 */  

int coswitch(old_cs, new_cs, fnc)
int *old_cs, *new_cs;
int fnc;
   {
   err_msg(401, (void*)0);
   }
