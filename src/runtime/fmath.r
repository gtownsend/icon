/*
 * fmath.r -- sin, cos, tan, acos, asin, atan, dtor, rtod, exp, log, sqrt
 */

/*
 * Most of the math ops are simple calls to underlying C functions,
 * sometimes with additional error checking to avoid and/or detect
 * various C runtime errors.
 */
#begdef MathOp(funcname,ccode,comment,pre,post)
#funcname "(r)" comment
function{1} funcname(x)

   if !cnv:C_double(x) then
      runerr(102, x)

   abstract {
      return real
      }
   inline {
      double y;
      pre		/* Pre math-operation range checking */
      errno = 0;
      y = ccode(x);
      post		/* Post math-operation C library error detection */
      return C_double y;
      }
end
#enddef


#define aroundone if (x < -1.0 || x > 1.0) {drunerr(205, x); errorfail;}
#define positive  if (x < 0)               {drunerr(205, x); errorfail;}

#define erange    if (errno == ERANGE)     runerr(204);
#define edom      if (errno == EDOM)       runerr(205);

MathOp(sin, sin,  ", x in radians.", ;, ;)
MathOp(cos, cos,  ", x in radians.", ;, ;)
MathOp(tan, tan,  ", x in radians.", ; , erange)
MathOp(acos,acos, ", x in radians.", aroundone, edom)
MathOp(asin,asin, ", x in radians.", aroundone, edom)
MathOp(exp, exp,  " - e^x.", ; , erange)
MathOp(sqrt,sqrt, " - square root of x.", positive, edom)
#define DTOR(x) ((x) * Pi / 180)
#define RTOD(x) ((x) * 180 / Pi)
MathOp(dtor,DTOR, " - convert x from degrees to radians.", ; , ;)
MathOp(rtod,RTOD, " - convert x from radians to degrees.", ; , ;)



"atan(r1,r2) -- r1, r2  in radians; if r2 is present, produces atan2(r1,r2)."

function{1} atan(x,y)

   if !cnv:C_double(x) then
      runerr(102, x)

   abstract {
      return real
      }
   if is:null(y) then
      inline {
         return C_double atan(x);
         }
   if !cnv:C_double(y) then
      runerr(102, y)
   inline {
      return C_double atan2(x,y);
      }
end


"log(r1,r2) - logarithm of r1 to base r2."

function{1} log(x,b)

   if !cnv:C_double(x) then
      runerr(102, x)

   abstract {
      return real
      }
   inline {
      if (x <= 0.0) {
         drunerr(205, x);
         errorfail;
         }
      }
   if is:null(b) then
      inline {
         return C_double log(x);
         }
   else {
      if !cnv:C_double(b) then
         runerr(102, b)
      body {
         static double lastbase = 0.0;
         static double divisor;

         if (b <= 1.0) {
            drunerr(205, b);
            errorfail;
            }
         if (b != lastbase) {
            divisor = log(b);
            lastbase = b;
            }
	 x = log(x) / divisor;
         return C_double x;
         }
      }
end

