/*
 *  bitcount(i) -- count the bits in an integer
 */

#include "icall.h"

int bitcount(argc, argv)
int argc;
descriptor *argv;
   {
   descriptor d;
   unsigned long v;
   int n;

   ArgInteger(1);			/* validate type */
   
   v = IntegerVal(argv[1]);		/* get value as unsigned long */
   n = 0;
   while (v != 0) {			/* while more bits to count */
      n += v & 1;			/*    check low-order bit */
      v >>= 1;				/*    shift off with zero-fill */
      }

   RetInteger(n);			/* return result */
   }
