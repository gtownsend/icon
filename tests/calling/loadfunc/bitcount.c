/*
 *  bitcount(i) -- count the bits in an integer
 */

#include "rt.h"

int bitcount(argc, argv)
int argc;
struct descrip *argv;
   {
   struct descrip d;
   unsigned long v;
   int n;

   if (argc < 1)
      return 101;			/* integer expected */
   
   if (!cnv_int(&argv[1], &d)) {
      argv[0] = argv[1];		/* offending value */
      return 101;			/* integer expected */
      }

   v = IntVal(argv[1]);			/* get value as unsigned long */
   n = 0;
   while (v != 0) {			/* while more bits to count */
      n += v & 1;			/*    check low-order bit */
      v >>= 1;				/*    shift off with zero-fill */
      }

   MakeInt(n, &argv[0]);		/* construct result integer */
   return 0;				/* success */
   }
