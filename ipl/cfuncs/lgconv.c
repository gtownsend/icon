/*
############################################################################
#
#	File:     lgconv.c
#
#	Subject:  Function to convert large integer to string
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
#  lgconv(I) converts a large integer into a string using a series of BCD
#  adds.  (In contrast, the Icon built-in string() function accomplishes
#  the same conversion using a series of divisions by 10.)
#
#  lgconv is typically 50% to 75% faster than string() on a Sun or Alpha.
#  For some reason it is as much as 125% SLOWER on a SGI 4/380.
#
#  lgconv(I) works for all integer values of I.  Small integers are
#  simply passed to string() for conversion.
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"
#include <math.h>
#include <string.h>

static void bcdadd(unsigned long lside[], unsigned long rside[], int n);



/* definitions copied from Icon source code */

typedef unsigned int DIGIT;
#define NB (WordSize / 2)	/* bits per digit */
#define B ((word)1 << NB)	/* bignum radix */

struct b_bignum {		/* large integer block */
   word title;			/*   T_Lrgint */
   word blksize;		/*   block size */
   word msd, lsd;		/*   most and least significant digits */
   int sign;			/*   sign; 0 positive, 1 negative */
   DIGIT digits[1];		/*   digits */
   };




int lgconv(argc, argv)		/*: convert large integer to string */
int argc;
descriptor *argv;
   {
#define BCDIGITS (2 * sizeof(long))	/* BCD digits per long */
   int nbig, ndec, nbcd, nchr, bcdlen, i, j, n, t;
   char tbuf[25], *o, *p;
   struct b_bignum *big;
   DIGIT d, *dgp;
   char *out;
   unsigned long b, *bp, *bcdbuf, *powbuf, *totbuf;

   t = IconType(argv[1]);
   if (t != 'I') {				/* if not large integer */
      ArgInteger(1);				/* must be a small one */
      sprintf(tbuf, "%ld", IntegerVal(argv[1]));
      RetString(tbuf);
      }

   big = (struct b_bignum *) argv[1].vword;	/* pointer to bignum struct */
   nbig = big->lsd - big->msd + 1;		/* number of bignum digits */
   ndec = nbig * NB * 0.3010299956639812 + 1;	/* number of decimal digits */
   nbcd = ndec / BCDIGITS + 1;			/* number of BCD longs */

   /* allocate string space for computation and output */
   nchr = sizeof(long) * (2 * nbcd + 1);
   out = alcstr(NULL, nchr);
   if (!out)
      Error(306);

   /* round up for proper alignment so we can overlay longword buffers */
   n = sizeof(long) - (long)out % sizeof(long);	/* adjustment needed */
   out += n;					/* increment address */
   nchr -= n;					/* decrement length */

   /* allocate computation buffers to overlay output string */
   bcdbuf = (unsigned long *) out;		/* BCD buffer area */
   bcdlen = 1;					/* start with just one BCD wd */
   totbuf = bcdbuf + nbcd - bcdlen;		/* BCD version of bignum */
   powbuf = totbuf + nbcd;			/* BCD powers of two */

   memset(bcdbuf, 0, 2 * nbcd * sizeof(long));	/* zero BCD buffers */
   powbuf[bcdlen-1] = 1;			/* init powbuf to 1 */

   /* compute BCD equivalent of the bignum value */
   dgp = &big->digits[big->lsd];
   for (i = 0; i < nbig; i++) {
      d = *dgp--;
      for (j = NB; j; j--) {
	 if (d & 1)				/* if bit is set in bignum */
	    bcdadd(totbuf, powbuf, bcdlen);	/* add 2^n to totbuf */
	 d >>= 1;
	 bcdadd(powbuf, powbuf, bcdlen);	/* double BCD power-of-two */
	 if (*powbuf >= (5LU << (WordSize-4))) {/* if too big to add */
	    bcdlen += 1;			/* grow buffers */
	    powbuf -= 1;
	    totbuf -= 1;
	    }
	 }
      }

   /* convert BCD to decimal characters */
   o = p = out + nchr;
   bp = totbuf + bcdlen;
   for (i = 0; i < bcdlen; i++) {
      b = *--bp;
      for (j = 0; j < BCDIGITS; j++) {
	 *--o = (b & 0xF) + '0';
	 b >>= 4;
	 }
      }

   /* trim leading zeroes, add sign, and return value */
   while (*o == '0' && o < p - 1)
      o++;
   if (big->sign)
      *--o = '-';
   RetAlcString(o, p - o);
   }



/*
 * bcdadd(lside,rside,n) -- compute lside += rside for n BCD longwords
 *
 * lside and rside are arrays of n unsigned longs holding BCD values,
 * with MSB in the first longword.  rside is added into lside in place.
 */

static void bcdadd(unsigned long lside[], unsigned long rside[], int n)
{
#define CSHIFT (WordSize - 4)
#if WordSize == 64
#define BIAS 0x6666666666666666u
#define MASK 0xF0F0F0F0F0F0F0F0u
#else
#define BIAS 0x66666666u
#define MASK 0xF0F0F0F0u
#endif
   unsigned long lword, rword, low, hgh, carry, icarry;

   lside += n;
   rside += n;
   carry = 0;

   while (n--) {
      lword = *--lside + BIAS;
      rword = *--rside + carry;
      hgh = (lword & MASK) + (rword & MASK);
      low = (lword & ~MASK) + (rword & ~MASK);
      while (icarry = (hgh & ~MASK) + (low & MASK)) {
	 hgh &= MASK;
	 low &= ~MASK;
	 carry |= icarry;
	 icarry = 0x16 * (icarry >> 4);
	 hgh += icarry & MASK;
	 low += icarry & ~MASK;
	 }
      carry = ((lword >> CSHIFT) + (rword >> CSHIFT) + (carry >> CSHIFT)) >> 4;
      *lside = hgh  + low + ((6 * carry) << CSHIFT) - BIAS;
      }
}
