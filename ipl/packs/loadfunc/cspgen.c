/*
 *  cspgen(image, cycle) - calculate next "cyclic space" generation
 *
 *  The image is considered a torus, with top and bottom connected directly
 *  and with sides connected using a shift of one row.
 */

/*
 *  internal buffer layout:
 *
 *	image header
 *	copy of last row
 *	original array
 *	copy of first row
 *	
 *  new array is stored atop old array, but directly after the header.
 */


#include <stdlib.h>
#include <string.h>
#include "icall.h"


int cspgen(int argc, descriptor *argv)
{
   int ulength, period, i;
   char *ustring, *udata, *cycle;
   char *old, *new;
   char o, x;

   int w, h, n;				/* width, height, total pixels */

   char hbuf[20];			/* image header buffer */
   int hlen;				/* header length */

   static char *ibuf;			/* image buffer */
   static int ilen;			/* buffer length */
   int ineed;				/* buffer length needed */

   static char map[256];		/* mapping from one char to next */

   /*
    * Get the parameters.
    */
   ArgString(1);			/* validate types */
   ArgString(2);
   ustring = StringAddr(argv[1]);	/* universe string and length */
   ulength = StringLen(argv[1]);
   cycle = StringAddr(argv[2]);		/* cycle and length */
   period = StringLen(argv[2]);
   sscanf(ustring, "%d", &w);		/* row width */

   /*
    * Build the generation mapping table.
    */
   map[cycle[period-1] & 0xFF] = cycle[0];	/* last maps to first */
   for (i = 1; i < period; i++)
      map[cycle[i-1] & 0xFF] = cycle[i];

   /*
    * Copy the image header (through the second comma) to hbuf.
    */
   old = ustring;
   new = hbuf;
   while ((*new++ = *old++) != ',')
      ;
   while ((*new++ = *old++) != ',')
      ;
   udata = old;
   hlen = udata - ustring;		/* header length */

   /*
    * Allocate the image buffer.
    */
   n = ulength - hlen;			/* number of pixels */
   if (n % w != 0)
      Error(205);
   h = n / w;				/* image height */

   ineed = hlen + n + 2 * w;		/* buffer size needed */
   if (ilen < ineed)
      if (!(ibuf = realloc(ibuf, ilen = ineed)))
	 Error(305);

   /*
    * Copy the image into the buffer.  Allow for the possibility that
    * the image already be *in* the buffer.
    */
   new = ibuf + hlen;
   old = new + w;
   memmove(old, udata, n);		/* main image, leaving room */
   memcpy(old - w, old + n - w, w);	/* dup last row first first */
   memcpy(old + n, old, w);		/* dup first row beyond last */

   /*
    * Create the new image.
    */
   memcpy(ibuf, hbuf, hlen);
   for (i = 0; i < n; i++) {
      o = *old;
      x = map[o & 0xFF];
      if (old[-1] == x || old[1] == x || old[-w] == x || old[w] == x)
	 o = x;
      *new++ = o;
      old++;
      }

   /*
    * Return the result.
    */
   RetConstStringN(ibuf, ulength);
}
