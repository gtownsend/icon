/*
############################################################################
#
#	File:     external.c
#
#	Subject:  Functions to demonstrate Icon external values
#
#	Author:   Gregg M. Townsend
#
#	Date:     September 17, 2007
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  These functions demonstrate the use of external values
#  and are also used in testing Icon.
#
#  extvtest0()		creates a minimal external type
#  extvtest1(i,j)	creates a custom external type
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"

/*
 * minimal external
 */

int extvtest0(int argc, descriptor argv[])	/*: create minimal external */
   {
   RetExternal(alcexternal(0, 0, 314159));
   }

/*
 * custom external with lots of trimmings
 */

/* custom external data block extends the standard block */
typedef struct myblock {
   externalblock eb;
   long v1;
   long v2;
   } myblock;

/* comparison function for sorting */
static long mycmp(externalblock *eb1, externalblock *eb2) {
   long d = ((myblock*)eb1)->v1 - ((myblock*)eb2)->v1;
   if (d == 0)
      d = ((myblock*)eb1)->v2 - ((myblock*)eb2)->v2;
   return d;
   }

/* type name of "custom" */
static descriptor myname(externalblock *eb) {
   static descriptor d = { 6, (long)"custom" };
   return d;
   }

/* custom formatting of image(e) */
static descriptor myimage(externalblock *eb) {
   myblock *b = (myblock*)eb;
   char buffer[100];
   int n = sprintf(buffer, "custom(%ld,%ld)", b->v1, b->v2);
   static descriptor d;
   
   d.dword = n;
   d.vword = (long)alcstr(buffer, n);
   return d;
   }

/* list of custom functions for constructor */
static funclist myfuncs = {
   mycmp,	/* cmp */
   0,		/* copy */
   myname,	/* name */
   myimage,	/* image */
   };

/* finally, the exported constructor function, extvtest1(i,j) */
int extvtest1(int argc, descriptor argv[])	/*: create custom external */
   {
   myblock *b;

   ArgInteger(1);
   ArgInteger(2);
   b = (myblock *)alcexternal(sizeof(myblock), &myfuncs, 0);
   b->v1 = IntegerVal(argv[1]);
   b->v2 = IntegerVal(argv[2]);
   RetExternal((externalblock*)b);
   }
