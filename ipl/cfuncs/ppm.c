/*
############################################################################
#
#	File:     ppm.c
#
#	Subject:  Functions to manipulate PPM files in memory
#
#	Author:   Gregg M. Townsend
#
#	Date:     November 17, 1997
#
############################################################################
#
#   This file is in the public domain.
#
############################################################################
#
#  These functions manipulate raw (P6) PPM image files in memory.
#  The images must not contain comment strings.
#
#  ppmwidth(s) -- return width of PPM image.
#  ppmheight(s) -- return height of PPM image.
#  ppmmax(s) -- return maximum value in PPM header.
#  ppmdata(s) -- return data portion of PPM image.
#
#  ppmimage(s,p,f) -- quantify image s using palette p, with flags f.
#     Returns an Icon image string.  Flag "o" selects ordered dithering.
#     Defaults:  p="c6",  f="o"
#
#  ppmstretch(s,lo,hi,max) -- apply contrast stretch operation
#     Returns a PPM string image that results from setting all
#     values <= lo to zero, all values >= hi to max, with values
#     between scaling linearly.  If hi = lo + 1, this becomes a
#     simple threshold operation.  If lo=0 and hi=ppmmax(s), this
#     simply scales an image to a new maximum.
#
#     Requirements: 0 <= lo < hi <= ppmmax(s), 1 <= max <= 255.
#     Defaults:	  lo=0, hi=ppmmax(s), max=255.
#
#  ppm3x3(s,a,b,c,d,e,f,g,h,i) -- apply 3x3 convolution to PPM image.
#     The matrix of real numbers [[a,b,c],[d,e,f],[g,h,i]] is used
#     as a transformation matrix applied independently to the three
#     color components of the image.
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/



#include "icall.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int palnum(descriptor *d);
char *rgbkey(int p, double r, double g, double b);



typedef struct {	/* ppminfo: struct describing a ppm image */
   int w, h;		/* width and height */
   int max;		/* maximum value */
   long npixels;	/* total number of pixels */
   long nbytes;		/* total number of pixels */
   char *data;		/* pointer to start of raw data; null indicates error */
} ppminfo;

static ppminfo ppmcrack(descriptor d);
static descriptor ppmalc(int w, int h, int max);
static char *rowextend(char *dst, char *src, int w, int nbr);
static int ppmrows(ppminfo hdr, int nbr, int (*func) (), long arg);
static int sharpenrow(char *a[], int w, int i, long max);
static int convrow(char *a[], int w, int i, long max);

static char *out;	/* general purpose global output pointer */



/* macros */

/* ArgPPM(int n, ppminfo hdr) -- validate arg n, init hdr */
#define ArgPPM(n,hdr) do {\
   ArgString(n); \
   hdr = ppmcrack(argv[n]); \
   if (!hdr.data) Fail; \
} while(0)

/* AlcResult(int w, h, max, ppminfo hdr) -- alc result string, init hdr */
/* WARNING -- can move other strings; refresh addresses from descriptors. */
#define AlcResult(w, h, max, hdr) do {\
   descriptor d = ppmalc(w, h, max); \
   if (d.vword == 0) Error(306); \
   hdr = ppmcrack(argv[0] = d); \
} while(0)



/* ppm info functions */

int ppmwidth(int argc, descriptor *argv)    /*: extract width of PPM string */
   {
   ppminfo hdr;

   ArgPPM(1, hdr);
   RetInteger(hdr.w);
   }

int ppmheight(int argc, descriptor *argv)    /*: extract height of PPM string */
   {
   ppminfo hdr;

   ArgPPM(1, hdr);
   RetInteger(hdr.h);
   }

int ppmmax(int argc, descriptor *argv)	    /*: extract max of PPM string */
   {
   ppminfo hdr;

   ArgPPM(1, hdr);
   RetInteger(hdr.max);
   }

int ppmdata(int argc, descriptor *argv)	    /*: extract data from PPM string */
   {
   ppminfo hdr;

   ArgPPM(1, hdr);
   RetAlcString(hdr.data, hdr.nbytes);
   }



/* ppmstretch(s,lo,hi) -- apply contrast stretch operation */

int ppmstretch(int argc, descriptor *argv) /*: stretch contrast of PPM string */
   {
   ppminfo src, dst;
   int lo, hi, max, i, v;
   float m;
   char *d, *s;

   ArgPPM(1, src);

   if (argc < 2 || IconType(argv[2]) == 'n')
      lo = 0;
   else {
      ArgInteger(2);
      lo = IntegerVal(argv[2]);
      if (lo < 0 || lo >= src.max)
         ArgError(2, 205);
      }

   if (argc < 3 || IconType(argv[3]) == 'n')
      hi = src.max;
   else {
      ArgInteger(3);
      hi = IntegerVal(argv[3]);
      if (hi <= lo || hi > src.max)
         ArgError(3, 205);
      }

   if (argc < 4 || IconType(argv[4]) == 'n')
      max = 255;
   else {
      ArgInteger(4);
      max = IntegerVal(argv[4]);
      if (max < 1 || max > 255)
         ArgError(4, 205);
      }

   m = (float)(max + 1) / (hi - lo);

   AlcResult(src.w, src.h, max, dst);
   src = ppmcrack(argv[1]);			/* may have moved */
   d = dst.data;
   s = src.data;
   for (i = 0; i < dst.nbytes; i++) {
      v = m * ((*s++ & 0xFF) - lo);
      if (v < 0) v = 0;
      else if (v > dst.max) v = dst.max;
      *d++ = v;
      }
   Return;
   }



/* ppmsharpen(s) -- apply fixed sharpening convolution  */

int ppmsharpen(int argc, descriptor *argv)	/*: sharpen a PPM string */
   {
   int rv;
   ppminfo src, dst;

   ArgPPM(1, src);
   AlcResult(src.w, src.h, src.max, dst);
   src = ppmcrack(argv[1]);			/* may have moved */

   out = dst.data;
   rv = ppmrows(src, 1, sharpenrow, src.max);
   if (rv == 0)
      Return;
   argv[0] = nulldesc;
   return rv;
   }

static int sharpenrow(char *a[], int w, int i, long max)
   {
   unsigned char *prev, *curr, *next;
   int v;

   prev = (unsigned char *) a[-1];
   curr = (unsigned char *) a[0];
   next = (unsigned char *) a[1];
   w *= 3;
   while (w--) {
      v = 2.0 * curr[0]
	- .10 * (prev[-3] + prev[3] + next[-3] + next[3])
	- .15 * (prev[0] + curr[-3] + curr[3] + next[0]);
      if (v < 0)
	 v = 0;
      else if (v > max)
	 v = max;
      *out++ = v;
      prev++;
      curr++;
      next++;
      }
   return 0;
   }



/* ppm3x3(s,a,b,c,d,e,f,g,h,i) -- apply 3x3 convolution matrix */

static float cells[9];

int ppm3x3(int argc, descriptor *argv)		/*: convolve PPM with matrix */
   {
   int rv, i;
   ppminfo src, dst;

   ArgPPM(1, src);
   for (i = 0; i < 9; i++) {
      ArgReal(i + 2);
      cells[i] = RealVal(argv[i + 2]);
      }

   AlcResult(src.w, src.h, src.max, dst);
   src = ppmcrack(argv[1]);			/* may have moved */

   out = dst.data;
   rv = ppmrows(src, 1, convrow, src.max);
   if (rv == 0)
      Return;
   argv[0] = nulldesc;
   return rv;
   }

static int convrow(char *a[], int w, int i, long max)
   {
   unsigned char *prev, *curr, *next;
   int v;

   prev = (unsigned char *) a[-1];
   curr = (unsigned char *) a[0];
   next = (unsigned char *) a[1];
   w *= 3;
   while (w--) {
      v = cells[0] * prev[-3] + cells[1] * prev[0] + cells[2] * prev[3]
        + cells[3] * curr[-3] + cells[4] * curr[0] + cells[5] * curr[3]
        + cells[6] * next[-3] + cells[7] * next[0] + cells[8] * next[3];
      if (v < 0)
	 v = 0;
      else if (v > max)
	 v = max;
      *out++ = v;
      prev++;
      curr++;
      next++;
      }
   return 0;
   }



/* ppmimage(s,p,f) -- quantify image s using palette p, returning Icon image. */

#define MDIM 16			/* dither matrix dimension */
#define MSIZE (MDIM * MDIM)	/* total size */

int ppmimage(int argc, descriptor *argv)	/*: dither PPM to Icon image */
   {
   int i, p, row, col, ir, ig, ib;
   double m, gd, r, g, b, dither[MSIZE], *dp, d;
   char *pname, *flags, *s, *t, *rv;
   ppminfo hdr;
   static double dmults[7] = {0., 1./3., 1./1., 1./2., 1./3., 1./4., 1./5.};
   static double gmults[7] = {0., 3./6., 1./2., 1./3., 1./4., 1./5., 1./6.};
   static unsigned char dfactor[MSIZE] = {
        0,128, 32,160,  8,136, 40,168,  2,130, 34,162, 10,138, 42,170,
      192, 64,224, 96,200, 72,232,104,194, 66,226, 98,202, 74,234,106,
       48,176, 16,144, 56,184, 24,152, 50,178, 18,146, 58,186, 26,154,
      240,112,208, 80,248,120,216, 88,242,114,210, 82,250,122,218, 90,
       12,140, 44,172,  4,132, 36,164, 14,142, 46,174,  6,134, 38,166,
      204, 76,236,108,196, 68,228,100,206, 78,238,110,198, 70,230,102,
       60,188, 28,156, 52,180, 20,148, 62,190, 30,158, 54,182, 22,150,
      252,124,220, 92,244,116,212, 84,254,126,222, 94,246,118,214, 86,
        3,131, 35,163, 11,139, 43,171,  1,129, 33,161,  9,137, 41,169,
      195, 67,227, 99,203, 75,235,107,193, 65,225, 97,201, 73,233,105,
       51,179, 19,147, 59,187, 27,155, 49,177, 17,145, 57,185, 25,153,
      243,115,211, 83,251,123,219, 91,241,113,209, 81,249,121,217, 89,
       15,143, 47,175,  7,135, 39,167, 13,141, 45,173,  5,133, 37,165,
      207, 79,239,111,199, 71,231,103,205, 77,237,109,197, 69,229,101,
       63,191, 31,159, 55,183, 23,151, 61,189, 29,157, 53,181, 21,149,
      255,127,223, 95,247,119,215, 87,253,125,221, 93,245,117,213, 85,
};

   ArgString(1);

   if (argc < 2 || IconType(argv[2]) == 'n') {
      p = 6;
      pname = "c6";
      }
   else {
      ArgString(2);
      p = palnum(&argv[2]);
      if (p == 0)  Fail;
      if (p == -1) ArgError(1, 103);
      pname = StringVal(argv[2]);
      }

   if (argc < 3 || IconType(argv[3]) == 'n')
      flags = "o";
   else {
      ArgString(3);
      flags = StringVal(argv[3]);
      }

   hdr = ppmcrack(argv[1]);
   if (!hdr.data)
      Fail;				/* PPM format error */

   if (!strchr(flags, 'o'))
      m = gd = 0.0;			/* no dithering */
   else if (p > 0) {
      m = dmults[p] - .0001;		/* color dithering magnitude */
      gd = gmults[p];			/* correction factor if gray input */
      }
   else {
      m = 1.0 / (-p - .9999);		/* grayscale dithering magnitude */
      gd = 1.0;				/* no correction needed */		
      }

   for (i = 0; i < MSIZE; i++)		/* build dithering table */
      dither[i] = m * (dfactor[i] / (double)(MSIZE)- 0.5);

   rv = alcstr(NULL, 10 + hdr.npixels);	/* allocate room for output string */
   if (!rv)
      Error(306);
   hdr = ppmcrack(argv[1]);		/* get addr again -- may have moved */
   sprintf(rv, "%d,%s,", hdr.w, pname);
   t = rv + strlen(rv);

   m = 1.0 / hdr.max;
   s = hdr.data;
   for (row = hdr.h; row > 0; row--) {
      dp = &dither[MDIM * (row & (MDIM - 1))];
      for (col = hdr.w; col > 0; col--) {
	 d = dp[col & (MDIM - 1)];
         ir = *s++ & 0xFF;
         ig = *s++ & 0xFF;
         ib = *s++ & 0xFF;
	 if (ir == ig && ig == ib) {
	    g = m * ig + gd * d;
            if (g < 0) g = 0; else if (g > 1) g = 1;
	    r = b = g;
	    }
	 else {
            r = m * ir + d;  if (r < 0) r = 0;  else if (r > 1) r = 1;
            g = m * ig + d;  if (g < 0) g = 0;  else if (g > 1) g = 1;
            b = m * ib + d;  if (b < 0) b = 0;  else if (b > 1) b = 1;
	    }
         *t++ = *(rgbkey(p, r, g, b));
         }
      }

   RetAlcString(rv, t - rv);
   }



/*************************  internal functions  *************************/



/*
 *  ppmalc(w, h, max) -- allocate new ppm image and initialize header
 *
 *  If allocation fails, the address in the returned descriptor is NULL.
 */
static descriptor ppmalc(int w, int h, int max)
   {
   char buf[32];
   descriptor d;

   sprintf(buf, "P6\n%d %d\n%d\n", w, h, max);
   d.dword = strlen(buf) + 3 * w * h;
   d.vword = (word)alcstr(NULL, d.dword);
   if (d.vword != 0)
      strcpy((void *)d.vword, buf);
   return d;
   }



/*  ppmcrack(d) -- crack PPM header, setting max=0 on error  */

static ppminfo ppmcrack(descriptor d)
   {
   int n;
   char *s;
   ppminfo info;
   static ppminfo zeroes;

   s = StringAddr(d);
   if (sscanf(s, "P6 %d %d %n", &info.w, &info.h, &n) < 2)
      return zeroes;			/* not a raw PPM file */

   /* can't scanf for "max" because it consumes too much trailing whitespace */
   info.max = 0;
   for (s += n; isspace(*s); s++)
      ;
   while (isdigit(*s))
      info.max = 10 * info.max + *s++ - '0';
   if (info.max == 0 || info.max > 255)
      return zeroes;			/* illegal max value for raw PPM */

   /* now consume exactly one more whitespace character */
   if (isspace(*s))
      s++;

   info.npixels = (long)info.w * (long)info.h;
   info.nbytes =  3 * info.npixels;
   if (s + info.nbytes > StringAddr(d) + StringLen(d))
      return zeroes;			/* file was truncated */

   info.data = s;
   return info;
   }



/*
 *  ppmrows(hdr, nbr, func, arg) -- extend rows and call driver function
 *
 *  Calls func(a, w, i, arg) for each row of the PPM image identified by hdr,
 *  where
 *	a is a pointer to a pointer to the first byte of the row (see below)
 *	w is the width of a row, in pixels
 *	i is the row number
 *	arg is passed along from the call to ppmrows
 *
 *  When nbr > 0, this indicates that func() needs to read up to nbr pixels
 *  above, below, left, and/or right of each source pixel; ppmrows copies
 *  and extends the rows to make this easy.  The argument "a" passed to func
 *  is a pointer to the center of an array of row pointers that extends by
 *  nbr rows in each direction.  That is, a[0] points to the current row;
 *  a[-1] points to the previous row, a[1] to the next row, and so on.
 *
 *  Each row is extended by nbr additional pixels in each direction by the
 *  duplication of the first and last pixels.  The pointers in the array "a"
 *  skip past the initial duplicates.  Thus a[0][0] is the first byte
 *  (the red byte) of the first pixel, a[0][-3] is its duplicate, and
 *  a[0][3] is the first byte of the second pixel of the row.
 *
 *  The idea behind all this complication is to make it easy to perform
 *  neighborhood operations.  See any caller of ppmrows for an example.
 *
 *  If ppmrows cannot allocate memory, it returns error code 305.
 *  If func returns nonzero, ppmrows returns that value immediately.
 *  Otherwise, ppmrows returns zero.
 */

static int ppmrows(ppminfo hdr, int nbr, int (*func) (), long arg)
   {
   char **a, *s;
   void *buf;
   int i, rv, np, row, rowlen;

   /* process nbr=0 without any copying */
   if (nbr <= 0) {
      s = hdr.data;
      for (row = 0; row < hdr.h; row++) {
	 rv = func(&s, hdr.w, row, arg);
	 if (rv != 0)
	    return rv;
	 s += 3 * hdr.w;
	 }
      return 0;
      }

   /* allocate memory for pointers and data */
   np = 2 * nbr + 1;			/* number of pointers */
   rowlen = 3 * (nbr + hdr.w + nbr);	/* length of one extended row */
   a = buf = malloc(np * sizeof(char *) + np * rowlen);
   if (buf == NULL)
      return 305;

   /* set pointers to row buffers */
   s = (char *)buf + np * sizeof(char *) + 3 * nbr;
   for (i = 0; i < np; i++) {
      *a++ = s;
      s += rowlen;
      }
   a -= nbr + 1;			/* point to center row */

   /* initialize buffers */
   for (i = -nbr; i < 0; i++)		/* duplicates of first row */
      rowextend(a[i], hdr.data, hdr.w, nbr);
   for (i = 0; i <= nbr; i++)		/* first nbr+1 rows */
      rowextend(a[i], hdr.data + 3 * i * hdr.w, hdr.w, nbr);

   /* iterate through rows */
   for (row = 0; row < hdr.h; row++) {

      /* call function for this row */
      rv = func(a, hdr.w, row, arg);
      if (rv != 0) {
	 free(buf);
	 return rv;
	 }

      /* rotate row pointers */
      s = a[-nbr];
      for (i = -nbr; i < nbr; i++)
	 a[i] = a[i+1];
      a[nbr] = s;

      /* replace oldest with new row */
      if (row + nbr < hdr.h)
	 rowextend(s, hdr.data + 3 * (row + nbr) * hdr.w, hdr.w, nbr);
      else
	 rowextend(s, hdr.data + 3 * (hdr.h - 1) * hdr.w, hdr.w, nbr);

      }

   free(buf);
   return 0;
   }



/*
 *  rowextend(dst, src, w, nbr) -- extend row on both ends
 *
 *  Copy w bytes from src to dst, extending both ends by nbr copies of
 *  the first/last 3-byte pixel.  w is the row width in pixels.
 *  Returns unextended dst pointer.
 */
static char *rowextend(char *dst, char *src, int w, int nbr)
   {
   char *s1, *s2, *d1, *d2;

   memcpy(dst, src, 3 * w);
   d1 = dst;
   d2 = dst + 3 * w;
   s1 = d1 + 3;
   s2 = d2 - 3;
   nbr *= 3;
   while (nbr--) {
      *--d1 = *--s1;
      *d2++ = *s2++;
      }
   return dst;
   }
