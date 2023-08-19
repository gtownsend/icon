/*
 *  ddump(a1, ...) -- descriptor dump
 *
 *  The arguments are dumped in hexadecimal on standard output.
 *
 *  This function does not require "icall.h".
 */

#include <stdio.h>

typedef struct {
   long dword;
   long vword;
} descriptor;

int ddump(int argc, descriptor *argv)
{
   int i, n;

   n = 2 * sizeof(long);
   for (i = 1; i <= argc; i++)
      printf("%d.  %0*lX  %0*lX\n", i, n, argv[i].dword, n, argv[i].vword);
   return 0;
}
