/*
############################################################################
#
#	File:     pack.c
#
#	Subject:  Functions to pack and unpack binary data
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
#  s := pack(value, flags, width)
#  x := unpack(string, flags)
#
#  Flag characters are as follows:
#
#     l -- little-endian [default]
#     b -- big-endian
#     n -- host platform's native packing order
#
#     i -- integer [default]
#     u -- unsigned integer
#     r -- real (host platform's native float or double format)
#
#  The default width is 4.
#
#  Integer values must fit in a standard Icon integer (not large integer).
#  Consequently, a word-sized value cannot have the high bit set if unsigned.
#  Floating values can only be converted to/from a string width matching
#  sizeof(float) or sizeof(double).
#
#  Size/type combinations that can't be handled produce errors.
#  Valid combinations produce failure if the value overflows.
#
#  Some of this code assumes a twos-complement architecture with 8-bit bytes.
#
############################################################################
#
#  Requires:  Dynamic loading
#
############################################################################
*/

#include "icall.h"
#include <string.h>

#define F_LTL 0x100	/* little-endian */
#define F_BIG 0x200	/* big-endian */
#define F_REV 0x400	/* internal flag: reversal needed */

#define F_INT 1		/* integer */
#define F_UNS 2		/* unsigned integer */
#define F_REAL 4	/* real */

#define DEF_WIDTH 4	/* default width */
#define MAX_WIDTH 256	/* maximum width */

static unsigned long testval = 1;
#define LNATIVE (*(char*)&testval)	/* true if machine is little-endian */

static int flags(char *s, int n);
static void *memrev(void *s1, void *s2, size_t n);

/*
 * pack(value, flags, width)
 */
int pack(int argc, descriptor argv[])	/*: pack integer into bytes */
   {
   int f, i, n, x;
   long v;
   unsigned char *s, obuf[MAX_WIDTH];
   union { float f; double d; unsigned char buf[MAX_WIDTH]; } u;

   /*
    * check arguments
    */
   if (argc == 0)
      Error(102);			/* no value given */

   if (argc > 1) {
      ArgString(2);
      if ((f = flags(StringAddr(argv[2]), StringLen(argv[2]))) == 0)
         ArgError(2, 205);		/* illegal flag string */
      }
   else
      f = flags("", 0);

   if (argc > 2) {
      ArgInteger(3);
      n = IntegerVal(argv[3]);
      if (n < 0 || n > MAX_WIDTH)
         ArgError(3, 205);		/* too long to handle */
      }
   else
      n = DEF_WIDTH;

   if (f & F_REAL) {

      /*
       * pack real value
       */
      ArgReal(1);
      if (n == sizeof(double))
         u.d = RealVal(argv[1]);
      else if (n == sizeof(float))
         u.f = RealVal(argv[1]);
      else
         ArgError(3, 205);		/* illegal length for real value */

      if (f & F_REV)
         RetStringN(memrev(obuf, u.buf, n), n);
      else
         RetStringN((char *)u.buf, n);
      }

   /*
    * pack integer value
    */
   ArgInteger(1);
   v = IntegerVal(argv[1]);		/* value */

   if (v >= 0)
      x = 0;				/* sign extension byte */
   else if (f & F_UNS)
      Fail;				/* invalid unsigned value */
   else
      x = (unsigned char) -1;

   for (s = obuf, i = 0; i < sizeof(long); i++)  {
      *s++ = v & 0xFF;			/* save in little-endian fashion */
      v = ((unsigned long)v) >> 8;
      }
   while (i++ < n)
      *s++ = x;				/* extend if > sizeof(long) */

   for (i = n; i < sizeof(long); i++)	/* check that all bits did fit */
      if (obuf[i] != x)
         Fail;				/* overflow */

   if (f & F_BIG)
      RetStringN(memrev(u.buf, obuf, n), n);
   else
      RetStringN((char *)obuf, n);
   }

/*
 * unpack(string, flags)
 */
int unpack(int argc, descriptor argv[])	/*: unpack integer from bytes */
   {
   int f, i, n, x;
   long v;
   unsigned char *s;
   union { float f; double d; unsigned char buf[MAX_WIDTH]; } u;

   /*
    * check arguments
    */
   ArgString(1);
   s = (unsigned char *)StringAddr(argv[1]);
   n = StringLen(argv[1]);
   if (n > MAX_WIDTH)
      ArgError(1, 205);			/* too long to handle */

   if (argc > 1) {
      ArgString(2);
      if ((f = flags(StringAddr(argv[2]), StringLen(argv[2]))) == 0)
         ArgError(2, 205);		/* illegal flag string */
      }
   else
      f = flags("", 0);

   if (f & F_REAL) {
      /*
       * unpack real value
       */
      if (f & F_REV)
         memrev(u.buf, s, n);
      else
         memcpy(u.buf, s, n);

      if (n == sizeof(double))
         RetReal(u.d);
      else if (n == sizeof(float))
         RetReal(u.f);
      else
         ArgError(1, 205);		/* illegal length for real value */
      }

   /*
    * unpack integer value
    */
   if (f & F_BIG)
      s = memrev(u.buf, s, n);		/* put in little-endian order */
   for (v = i = 0; i < n && i < sizeof(long); i++)
      v |= *s++ << (8 * i);		/* pack into a long */

   if (v >= 0)
      x = 0;				/* sign extension byte */
   else if (f & F_UNS)
      Fail;				/* value overflows as unsigned */
   else
      x = (unsigned char) -1;

   for (; i < n; i++)			/* check bytes beyond sizeof(long) */
      if (*s++ != x)
         Fail;				/* value overflows a long */

   RetInteger(v);			/* return value */
   }


/*
 * flags(addr, len) -- interpret flag string, return 0 if error
 */
static int flags(char *s, int n)
   {
   int f = 0;

   while (n--) switch(*s++) {
      case 'l':  f |= F_LTL;				break;
      case 'b':  f |= F_BIG;				break;
      case 'n':  f |= (LNATIVE ? F_LTL : F_BIG);	break;
      case 'i':  f |= F_INT;				break;
      case 'u':  f |= F_UNS + F_INT;			break;
      case 'r':  f |= F_REAL;				break;
      default:	 return 0;
      }

   if (((f & F_LTL) && (f & F_BIG)) | ((f & F_INT) && (f & F_REAL)))
      return 0;				/* illegal conflict */

   if (!(f & F_BIG))
      f |= F_LTL;			/* default packing is little-endian */
   if (!(f & F_REAL))
      f |= F_INT;			/* default type is integer */

   if (f & (LNATIVE ? F_BIG : F_LTL))
      f |= F_REV;			/* set flag if non-native mode */

   return f;
   }


/*
 * memrev(s1, s2, n) -- copy reversal of s2 into s1, returning s1
 */
static void *memrev(void *s1, void *s2, size_t n)
   {
   unsigned char *c1 = s1;
   unsigned char *c2 = (unsigned char *)s2 + n;
   while (n-- > 0)
      *c1++ = *--c2;
   return s1;
   }
