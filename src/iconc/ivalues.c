/*
 * ivalues.c - routines for manipulating Icon values.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ccode.h"
#include "cproto.h"
#include "cglobals.h"


/*
 * iconint - convert the string representation of an Icon integer to a C long. 
 *   Return -1 if the number is too big and large integers are supported.
 */
long iconint(image)
char *image;
   {
   register int c;
   register int r;
   register char *s;
   long n, n1;
   int overflow;

   s = image;
   overflow = 0;
   n = 0L;
   while ((c = *s++) >= '0' && c <= '9') {
      n1 = n * 10 + (c - '0');
      if (n != n1 / 10)
         overflow = 1;
      n = n1;
      }
   if (c == 'r' || c == 'R') {
      r = n;
      n = 0L;
      while ((c = *s++) != '\0') {
         n1 = n * r + tonum(c);
         if (n != n1 / r)
            overflow = 1;
         n = n1;
         }
      }
   if (overflow)
      if (largeints)
         n = -1;
      else
         tfatal("large integer option required", image);
   return n;
   }
