/*
############################################################################
#
#	File:     external.c
#
#	Subject:  Functions to demonstrate Icon external values
#
#	Author:   Gregg M. Townsend
#
#	Date:     May 29, 2013
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
 * custom external holding a null-terminated string and a trivial checksum
 */

/* data area */
typedef struct sdata {
   unsigned short cksum;
   char string[];
   } sdata;

/* type name returns "xstr" */
static int sname(int argc, descriptor argv[]) {
   RetConstStringN("xstr", 4);
   }

/* image returns "xstr_N(cksum:string)" with no special string escapes */
static int simage(int argc, descriptor argv[]) {
   externalblock *xb = ExternalBlock(argv[1]);
   sdata *d = (sdata*) xb->data;
   char buffer[1000];	/* not robust against huge strings */
   RetStringN(buffer,
      sprintf(buffer, "xstr_%ld(%05d:%s)", xb->id, d->cksum, d->string));
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
   ArgString(1);
   int slen = StringLen(argv[1]);
   externalblock *xb = alcexternal(
      sizeof(externalblock) + sizeof(sdata) + slen + 1, &sfuncs, 0);
   sdata *d = (sdata*) xb->data;
   memcpy(d->string, StringAddr(argv[1]), slen);
   d->string[slen] = '\0';
   int cksum = 0;
   char *p;
   for (p = d->string; *p; p++)
      cksum = 37 * cksum + (unsigned char) *p;
   d->cksum = cksum;
   RetExternal(xb);
   }


/*
 * custom real-valued external with lots of trimmings
 */

/* data area */
typedef struct rdata {
   float value;
   } rdata;

/* comparison function for sorting */
static int rcmp(int argc, descriptor argv[]) {
   externalblock *xb1 = ExternalBlock(argv[1]);
   externalblock *xb2 = ExternalBlock(argv[2]);
   rdata *data1 = (rdata*) xb1->data;
   rdata *data2 = (rdata*) xb2->data;
   if (data1->value < data2->value) RetInteger(-1);
   if (data1->value > data2->value) RetInteger(+1);
   if (xb1->id < xb2->id) RetInteger(-1);
   if (xb1->id > xb2->id) RetInteger(+1);
   RetInteger(0);
   }

/* copy function duplicates block, getting new serial number */
static int rcopy(int argc, descriptor argv[]) {
   externalblock *xb = ExternalBlock(argv[1]);
   RetExternal(alcexternal(
      sizeof(externalblock) + sizeof(rdata), xb->funcs, xb->data));
   }

/* type name returns "xreal" */
static int rname(int argc, descriptor argv[]) {
   RetConstStringN("xreal", 5);
   }

/* image returns "xreal_N(V)" */
static int rimage(int argc, descriptor argv[]) {
   externalblock *xb = ExternalBlock(argv[1]);
   rdata *d = (rdata*) xb->data;
   char buffer[100];
   RetStringN(buffer,
      sprintf(buffer, "xreal_%ld(%.1f)", xb->id, d->value));
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
   ArgReal(1);
   float v = RealVal(argv[1]);
   RetExternal(alcexternal(
      sizeof(externalblock) + sizeof(rdata), &rfuncs, &v));
   }
