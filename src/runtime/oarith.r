/*
 * File: oarith.r
 *  Contents: arithmetic operators + - * / % ^.  Auxiliary routines
 *   iipow, ripow.
 *
 * The arithmetic operators all follow a canonical conversion
 * protocol encapsulated in the macro ArithOp.
 */

int over_flow = 0;

#begdef ArithOp(icon_op, func_name, c_int_op, c_real_op)

   operator{1} icon_op func_name(x, y)
      declare {
#ifdef LargeInts
         tended struct descrip lx, ly;
#endif					/* LargeInts */
	 C_integer irslt;
         }
     arith_case (x, y) of {
         C_integer: {
            abstract {
               return integer
               }
            inline {
               extern int over_flow;
               c_int_op(x,y);
               }
            }
         integer: { /* large integers only */
            abstract {
               return integer
               }
            inline {
               big_ ## c_int_op(x,y);
               }
            }
         C_double: {
            abstract {
               return real
               }
            inline {
               c_real_op(x, y);
               }
            }
         }
end

#enddef

/*
 * x / y
 */

#begdef big_Divide(x,y)
{
   if ( ( Type ( y ) == T_Integer ) && ( IntVal ( y ) == 0 ) )
      runerr(201);  /* Divide fix */

   if (bigdiv(&x,&y,&result) == Error) /* alcbignum failed */
      runerr(0);
   return result;
}
#enddef
#begdef Divide(x,y)
{
   if ( y == 0 )
      runerr(201);  /* divide fix */

   irslt = div3(x,y);
   if (over_flow) {
#ifdef LargeInts
      MakeInt(x,&lx);
      MakeInt(y,&ly);
      if (bigdiv(&lx,&ly,&result) == Error) /* alcbignum failed */
	 runerr(0);
      return result;
#else					/* LargeInts */
      runerr(203);
#endif					/* LargeInts */
      }
   else return C_integer irslt;
}
#enddef
#begdef RealDivide(x,y)
{
   double z;

   if (y == 0.0)
      runerr(204);
   z = x / y;
   return C_double z;
}
#enddef


ArithOp( / , divide , Divide , RealDivide)

/*
 * x - y
 */

#begdef big_Sub(x,y)
{
   if (bigsub(&x,&y,&result) == Error) /* alcbignum failed */
      runerr(0);
   return result;
}
#enddef

#begdef Sub(x,y)
   irslt = sub(x,y);
   if (over_flow) {
#ifdef LargeInts
      MakeInt(x,&lx);
      MakeInt(y,&ly);
      if (bigsub(&lx,&ly,&result) == Error) /* alcbignum failed */
         runerr(0);
      return result;
#else					/* LargeInts */
      runerr(203);
#endif					/* LargeInts */
      }
   else return C_integer irslt;
#enddef

#define RealSub(x,y) return C_double (x - y);

ArithOp( - , minus , Sub , RealSub)


/*
 * x % y
 */
#define Abs(x) ((x) > 0 ? (x) : -(x))

#begdef big_IntMod(x,y)
{
   if ( ( Type ( y ) == T_Integer ) && ( IntVal ( y ) == 0 ) ) {
      irunerr(202,0);
      errorfail;
      }
   if (bigmod(&x,&y,&result) == Error)
      runerr(0);
   return result;
}
#enddef

#begdef IntMod(x,y)
{
   irslt = mod3(x,y);
   if (over_flow) {
      irunerr(202,y);
      errorfail;
      }
   return C_integer irslt;
}
#enddef

#begdef RealMod(x,y)
{
   double d;

   if (y == 0.0)
      runerr(204);

   d = fmod(x, y);
   /* d must have the same sign as x */
   if (x < 0.0) {
      if (d > 0.0) {
         d -= Abs(y);
         }
      }
   else if (d < 0.0) {
      d += Abs(y);
      }
   return C_double d;
}
#enddef

ArithOp( % , mod , IntMod , RealMod)

/*
 * x * y
 */

#begdef big_Mpy(x,y)
{
   if (bigmul(&x,&y,&result) == Error)
      runerr(0);
   return result;
}
#enddef

#begdef Mpy(x,y)
   irslt = mul(x,y);
   if (over_flow) {
#ifdef LargeInts
      MakeInt(x,&lx);
      MakeInt(y,&ly);
      if (bigmul(&lx,&ly,&result) == Error) /* alcbignum failed */
         runerr(0);
      return result;
#else					/* LargeInts */
      runerr(203);
#endif					/* LargeInts */
      }
   else return C_integer irslt;
#enddef


#define RealMpy(x,y) return C_double (x * y);

ArithOp( * , mult , Mpy , RealMpy)


"-x - negate x."

operator{1} - neg(x)
   if cnv:(exact)C_integer(x) then {
      abstract {
         return integer
         }
      inline {
	    C_integer i;
	    extern int over_flow;

	    i = neg(x);
	    if (over_flow) {
#ifdef LargeInts
	       struct descrip tmp;
	       MakeInt(x,&tmp);
	       if (bigneg(&tmp, &result) == Error)  /* alcbignum failed */
	          runerr(0);
               return result;
#else					/* LargeInts */
	       irunerr(203,x);
               errorfail;
#endif					/* LargeInts */
               }
         return C_integer i;
         }
      }
#ifdef LargeInts
   else if cnv:(exact) integer(x) then {
      abstract {
         return integer
         }
      inline {
	 if (bigneg(&x, &result) == Error)  /* alcbignum failed */
	    runerr(0);
	 return result;
         }
      }
#endif					/* LargeInts */
   else {
      if !cnv:C_double(x) then
         runerr(102, x)
      abstract {
         return real
         }
      inline {
         double drslt;
	 drslt = -x;
         return C_double drslt;
         }
      }
end


"+x - convert x to a number."
/*
 *  Operational definition: generate runerr if x is not numeric.
 */
operator{1} + number(x)
   if cnv:(exact)C_integer(x) then {
       abstract {
          return integer
          }
       inline {
          return C_integer x;
          }
      }
#ifdef LargeInts
   else if cnv:(exact) integer(x) then {
       abstract {
          return integer
          }
       inline {
          return x;
          }
      }
#endif					/* LargeInts */
   else if cnv:C_double(x) then {
       abstract {
          return real
          }
       inline {
          return C_double x;
          }
      }
   else
      runerr(102, x)
end

/*
 * x + y
 */

#begdef big_Add(x,y)
{
   if (bigadd(&x,&y,&result) == Error)
      runerr(0);
   return result;
}
#enddef

#begdef Add(x,y)
   irslt = add(x,y);
   if (over_flow) {
#ifdef LargeInts
      MakeInt(x,&lx);
      MakeInt(y,&ly);
      if (bigadd(&lx, &ly, &result) == Error)  /* alcbignum failed */
	 runerr(0);
      return result;
#else					/* LargeInts */
      runerr(203);
#endif					/* LargeInts */
      }
   else return C_integer irslt;
#enddef

#define RealAdd(x,y) return C_double (x + y);

ArithOp( + , plus , Add , RealAdd)


"x ^ y - raise x to the y power."

operator{1} ^ powr(x, y)
   if cnv:(exact)C_integer(y) then {
      if cnv:(exact)integer(x) then {
	 abstract {
	    return integer
	    }
	 inline {
#ifdef LargeInts
	    tended struct descrip ly;
	    MakeInt ( y, &ly );
	    if (bigpow(&x, &ly, &result) == Error)  /* alcbignum failed */
	       runerr(0);
	    return result;
#else
	    extern int over_flow;
	    C_integer r = iipow(IntVal(x), y);
	    if (over_flow)
	       runerr(203);
	    return C_integer r;
#endif
	   }
	 }
      else {
	 if !cnv:C_double(x) then
	    runerr(102, x)
	 abstract {
	    return real
	    }
	 inline {
	    if (ripow( x, y, &result) ==  Error)
	       runerr(0);
	    return result;
	    }
	 }
      }
#ifdef LargeInts
   else if cnv:(exact)integer(y) then {
      if cnv:(exact)integer(x) then {
	 abstract {
	    return integer
	    }
	 inline {
	    if (bigpow(&x, &y, &result) == Error)  /* alcbignum failed */
	       runerr(0);
	    return result;
	    }
	 }
      else {
	 if !cnv:C_double(x) then
	    runerr(102, x)
	 abstract {
	    return real
	    }
	 inline {
	    if ( bigpowri ( x, &y, &result ) == Error )
	       runerr(0);
	    return result;
	    }
	 }
      }
#endif					/* LargeInts */
   else {
      if !cnv:C_double(x) then
	 runerr(102, x)
      if !cnv:C_double(y) then
	 runerr(102, y)
      abstract {
	 return real
	 }
      inline {
	 if (x == 0.0 && y < 0.0)
	     runerr(204);
	 if (x < 0.0)
	    runerr(206);
	 return C_double pow(x,y);
	 }
      }
end

#if COMPILER || !(defined LargeInts)
/*
 * iipow - raise an integer to an integral power.
 */
C_integer iipow(n1, n2)
C_integer n1, n2;
   {
   C_integer result;

   /* Handle some special cases first */
   over_flow = 0;
   switch ( n1 ) {
      case 1:
	 return 1;
      case -1:
	 /* Result depends on whether n2 is even or odd */
	 return ( n2 & 01 ) ? -1 : 1;
      case 0:
	 if ( n2 <= 0 )
	    over_flow = 1;
	 return 0;
      default:
	 if (n2 < 0)
	    return 0;
      }

   result = 1L;
   for ( ; ; ) {
      if (n2 & 01L)
	 {
	 result = mul(result, n1);
	 if (over_flow)
	    return 0;
	 }

      if ( ( n2 >>= 1 ) == 0 ) break;
      n1 = mul(n1, n1);
      if (over_flow)
	 return 0;
      }
   over_flow = 0;
   return result;
   }
#endif					/* COMPILER || !(defined LargeInts) */


/*
 * ripow - raise a real number to an integral power.
 */
int ripow(r, n, drslt)
double r;
C_integer n;
dptr drslt;
   {
   double retval;

   if (r == 0.0 && n <= 0)
      ReturnErrNum(204, Error);
   if (n < 0) {
      /*
       * r ^ n = ( 1/r ) * ( ( 1/r ) ^ ( -1 - n ) )
       *
       * (-1) - n never overflows, even when n == MinLong.
       */
      n = (-1) - n;
      r = 1.0 / r;
      retval = r;
      }
   else
      retval = 1.0;

   /* multiply retval by r ^ n */
   while (n > 0) {
      if (n & 01L)
	 retval *= r;
      r *= r;
      n >>= 1;
      }
   Protect(BlkLoc(*drslt) = (union block *)alcreal(retval), return Error);
   drslt->dword = D_Real;
   return Succeeded;
   }
