/*
 * dlrgint.c - versions of "large integer" routines for compiled programs
 *   that do not support large integers.
 */
#define COMPILER 1
#include "../h/rt.h"

/*
 *****************************************************************
 *
 * Routines in the first set are only called when large integers
 *  exist and thus these versions will never be called. They need
 *  only have the correct signature and compile without error.
 */

/*
 *  bignum -> file
 */
void bigprint(f, da)
FILE *f;
dptr da;
   {
   }

/*
 *  bignum -> real
 */
double bigtoreal(da)
dptr da;
   {
   return 0.0;
   }

/*
 *  bignum -> string
 */
int bigtos(da, dx)
dptr da, dx;
   {
   return 0;
   }

/*
 *  da -> dx
 */
int cpbignum(da, dx)
dptr da, dx;
   {
   return 0;
   }

/*
 *  da / db -> dx
 */
int bigdiv(da, db, dx)
dptr da, db, dx;
   {
   return 0;
   }

/*
 *  da % db -> dx
 */
int bigmod(da, db, dx)
dptr da, db, dx;
   {
   return 0;
   }

/*
 *  iand(da, db) -> dx
 */
int bigand(da, db, dx)
dptr da, db, dx;
   {
   return 0;
   }

/*
 *  ior(da, db) -> dx
 */
int bigor(da, db, dx)
dptr da, db, dx;
   {
   return 0;
   }

/*
 *  xor(da, db) -> dx
 */
int bigxor(da, db, dx)
dptr da, db, dx;
   {
   return 0;
   }

/*
 *  negative if da < db
 *  zero if da == db
 *  positive if da > db
 */
word bigcmp(da, db)
dptr da, db;
   {
   return (word)0;
   }

/*
 *  ?da -> dx
 */
int bigrand(da, dx)
dptr da, dx;
   {
   return 0;
   }

/*
 *************************************************************
 *
 * The following routines are called when overflow has occurred
 *   during ordinary arithmetic.
 */

/*
 *  da + db -> dx
 */
int bigadd(da, db, dx)
dptr da, db;
dptr dx;
   {
   t_errornumber = 203;
   t_errorvalue = nulldesc;
   t_have_val = 0;
   return Error;
   }

/*
 *  da * db -> dx
 */
int bigmul(da, db, dx)
dptr da, db, dx;
   {
   t_errornumber = 203;
   t_errorvalue = nulldesc;
   t_have_val = 0;
   return Error;
   }

/*
 *  -i -> dx
 */
int bigneg(da, dx)
dptr da, dx;
   {
   t_errornumber = 203;
   t_errorvalue = nulldesc;
   t_have_val = 0;
   return Error;
   }

/*
 *  da - db -> dx
 */
int bigsub(da, db, dx)
dptr da, db, dx;
   {
   t_errornumber = 203;
   t_errorvalue = nulldesc;
   t_have_val = 0;
   return Error;
   }

/*
 * ********************************************************
 *
 * The remaining routines each requires different handling.
 */

/*
 *  real -> bignum
 */
int realtobig(da, dx)
dptr da, dx;
   {
   return Failed;  /* conversion cannot be done */
   }

/*
 *  da ^ db -> dx
 */
int bigpow(da, db, dx)
dptr da, db, dx;
   {
   C_integer r;
   extern int over_flow;

   /*
    * Just do ordinary interger exponentiation and check for overflow.
    */
   r = iipow(IntVal(*da), IntVal(*db));
   if (over_flow) {
      k_errornumber = 203;
      k_errortext = "";
      k_errorvalue = nulldesc;
      have_errval = 0;
      return Error;
      }
   MakeInt(r, dx);
   return Succeeded;
   }

/*
 *  string -> bignum
 */
word bigradix(sign, r, s, end_s, result)
int sign;                      /* '-' or not */
int r;                          /* radix 2 .. 36 */
char *s, *end_s;                        /* input string */
union numeric *result;          /* output T_Integer or T_Lrgint */
   {
   /*
    * Just do string to ordinary integer.
    */
   return radix(sign, r, s, end_s, result);
   }

/*
 *  bigshift(da, db) -> dx
 */
int bigshift(da, db, dx)
dptr da, db, dx;
   {
   uword ci;                  /* shift in 0s, even if negative */
   C_integer cj;

   /*
    * Do an ordinary shift - note that db is always positive when this
    *   routine is called.
    */
   ci = (uword)IntVal(*da);
   cj = IntVal(*db);
   /*
    * Check for a shift of WordSize or greater; return an explicit 0 because
    *  this is beyond C's defined behavior.  Otherwise shift as requested.
    */
   if (cj >= WordBits)
      ci = 0;
   else
      ci <<= cj;
   MakeInt(ci, dx);
   return Succeeded;
   }
