#include "../h/gsupport.h"

/*
 * millisec - returns execution time in milliseconds. Time is measured
 *  from the function's first call. The granularity of the time is
 *  generally more than one millisecond and on some systems it my only
 *  be accurate to the second.
 */

#if UNIX

/*
 * For some unfathomable reason, the Open Group's "Single Unix Specification"
 *  requires that the ANSI C clock() function be defined in units of 1/1000000
 *  second.  This means that the result overflows a 32-bit signed clock_t
 *  value only about 35 minutes.  So, under UNIX, we use the POSIX standard
 *  times() function instead.
 */

static long cptime()
   {
   struct tms tp;
   times(&tp);
   return (long) (tp.tms_utime + tp.tms_stime);
   }

long millisec()
   {
   static long starttime = -2;
   long t;

   t = cptime();
   if (starttime == -2)
      starttime = t;
   return (long) ((1000.0 / CLK_TCK) * (t - starttime));
   }

#else					/* UNIX */

/*
 * On anything other than UNIX, just use the ANSI C clock() function.
 */

long millisec()
   {
   static clock_t starttime = -2;
   clock_t t;

   t = clock();
   if (starttime == -2)
      starttime = t;
   return (long) ((1000.0 / CLOCKS_PER_SEC) * (t - starttime));
   }

#endif					/* UNIX */
