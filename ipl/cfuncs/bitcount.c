/*
############################################################################
#
#	File:     bitcount.c
#
#	Subject:  Function to count bits in an integer
#
#	Author:   Gregg M. Townsend
#
#	Date:     April 9, 1996
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  bitcount(i) returns the number of bits that are set in the integer i.
#  It works only for "normal" integers, not large integers.
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"

int bitcount(int argc, descriptor *argv) /*: count bits in an integer */
   {
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
