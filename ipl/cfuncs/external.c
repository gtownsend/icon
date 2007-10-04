/*
############################################################################
#
#	File:     external.c
#
#	Subject:  Functions to demonstrate Icon external values
#
#	Author:   Gregg M. Townsend
#
#	Date:     October 3, 2007
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  These functions demonstrate the use of external values.
#
#  extv0()	creates a minimal external type
#  extv1(r)	creates a fully customized external type
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"

/*
 * minimal external type with no parameters
 */
int extv0(int argc, descriptor argv[])	/*: create minimal external value */
   {
   RetExternal(alcexternal(0, 0, 0x112358));
   }

/*
 * custom external with lots of trimmings
 */

/* custom external data block extends the standard block */
typedef struct myblock {
   externalblock eb;
   float value;
   } myblock;

/* comparison function for sorting */
static int mycmp(int argc, descriptor argv[]) {
   myblock *eb1 = (myblock*)ExternalBlock(argv[1]);
   myblock *eb2 = (myblock*)ExternalBlock(argv[2]);
   if (eb1->value < eb2->value) RetInteger(-1);
   if (eb1->value > eb2->value) RetInteger(+1);
   if (eb1->eb.id < eb2->eb.id) RetInteger(-1);
   if (eb1->eb.id > eb2->eb.id) RetInteger(+1);
   RetInteger(0);
   }

/* copy function duplicates block, getting new serial number */
static int mycopy(int argc, descriptor argv[]) {
   externalblock *b = ExternalBlock(argv[1]);
   myblock *old = (myblock*)b;
   myblock *new = (myblock *)alcexternal(sizeof(myblock), b->funcs, 0);
   new->value = old->value;
   RetExternal((externalblock*)new);
   }

/* type name returns "custom" */
static int myname(int argc, descriptor argv[]) {
   RetConstStringN("custom", 6);
   }

/* image returns "custom_N(V)" */
static int myimage(int argc, descriptor argv[]) {
   myblock *b = (myblock*)ExternalBlock(argv[1]);
   char buffer[100];
   RetStringN(buffer, sprintf(buffer, "custom_%ld(%.1f)", b->eb.id, b->value));
   }

/* list of custom functions for constructor */
static funclist myfuncs = {
   mycmp,	/* cmp */
   mycopy,	/* copy */
   myname,	/* name */
   myimage,	/* image */
   };

/* finally, the exported constructor function, extv1(r) */
int extv1(int argc, descriptor argv[])	/*: create custom external */
   {
   myblock *new;

   ArgReal(1);
   new = (myblock *)alcexternal(sizeof(myblock), &myfuncs, 0);
   new->value = RealVal(argv[1]);
   RetExternal((externalblock*)new);
   }
