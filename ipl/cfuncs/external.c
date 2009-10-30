/*
############################################################################
#
#	File:     external.c
#
#	Subject:  Functions to demonstrate Icon external values
#
#	Author:   Gregg M. Townsend
#
#	Date:     October 29, 2009
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  These functions demonstrate the use of external values.
#
#  extxmin()	creates a minimal external type
#  extxstr(s)	creates an external hold a string and trivial checksum
#  extxreal(r)	creates a fully customized external type holding a real value
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include <string.h>
#include "icall.h"

/*
 * minimal external type with no parameters
 */
int extxmin(int argc, descriptor argv[])   /*: create minimal external value */
   {
   RetExternal(alcexternal(0, 0, 0));
   }

/*
 * custom external holding a string and a trivial checksum
 */

/* custom external data block extends the standard block */
typedef struct sblock {
   externalblock eb;
   unsigned short cksum;
   char string[];
   } sblock;

/* type name returns "xstr" */
static int sname(int argc, descriptor argv[]) {
   RetConstStringN("xstr", 4);
   }

/* image returns "xstr_N(cksum:string)" with no special string escapes */
static int simage(int argc, descriptor argv[]) {
   sblock *b = (sblock*)ExternalBlock(argv[1]);
   char buffer[1000];	/* not robust against huge strings */
   RetStringN(buffer,
      sprintf(buffer, "xstr_%ld(%05d:%s)", b->eb.id, b->cksum, b->string));
   }

/* list of custom functions for constructor */
static funclist sfuncs = {
   NULL,	/* cmp */
   NULL,	/* copy */
   sname,	/* name */
   simage,	/* image */
   };

/* finally, the exported constructor function, extxstr(s) */
int extxstr(int argc, descriptor argv[])   /*: create string-valued external */
   {
   sblock *new;
   char *p;
   int slen;

   ArgString(1);
   slen = StringLen(argv[1]);
   new = (sblock *)alcexternal(sizeof(sblock) + slen + 1, &sfuncs, 0);
   memcpy(new->string, StringAddr(argv[1]), slen);
   new->string[slen] = '\0';
   int cksum = 0;
   for (p = new->string; *p; p++)
       cksum = 37 * cksum + (unsigned char) *p;
   new->cksum = cksum;
   RetExternal((externalblock*)new);
   }


/*
 * custom real-valued external with lots of trimmings
 */

/* custom external data block extends the standard block */
typedef struct rblock {
   externalblock eb;
   float value;
   } rblock;

/* comparison function for sorting */
static int rcmp(int argc, descriptor argv[]) {
   rblock *eb1 = (rblock*)ExternalBlock(argv[1]);
   rblock *eb2 = (rblock*)ExternalBlock(argv[2]);
   if (eb1->value < eb2->value) RetInteger(-1);
   if (eb1->value > eb2->value) RetInteger(+1);
   if (eb1->eb.id < eb2->eb.id) RetInteger(-1);
   if (eb1->eb.id > eb2->eb.id) RetInteger(+1);
   RetInteger(0);
   }

/* copy function duplicates block, getting new serial number */
static int rcopy(int argc, descriptor argv[]) {
   externalblock *b = ExternalBlock(argv[1]);
   rblock *old = (rblock*)b;
   rblock *new = (rblock *)alcexternal(sizeof(rblock), b->funcs, 0);
   new->value = old->value;
   RetExternal((externalblock*)new);
   }

/* type name returns "xreal" */
static int rname(int argc, descriptor argv[]) {
   RetConstStringN("xreal", 5);
   }

/* image returns "xreal_N(V)" */
static int rimage(int argc, descriptor argv[]) {
   rblock *b = (rblock*)ExternalBlock(argv[1]);
   char buffer[100];
   RetStringN(buffer,
      sprintf(buffer, "xreal_%ld(%.1f)", b->eb.id, b->value));
   }

/* list of custom functions for constructor */
static funclist rfuncs = {
   rcmp,	/* cmp */
   rcopy,	/* copy */
   rname,	/* name */
   rimage,	/* image */
   };

/* finally, the exported constructor function, extxreal(r) */
int extxreal(int argc, descriptor argv[])    /*: create real-valued external */
   {
   rblock *new;

   ArgReal(1);
   float v = RealVal(argv[1]);
   new = (rblock *)alcexternal(sizeof(rblock), &rfuncs, &v);
   RetExternal((externalblock*)new);
   }
