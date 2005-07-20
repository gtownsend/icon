#include "../h/gsupport.h"

/*
 * millisec - returns execution time in milliseconds. Time is measured
 *  from the function's first call. The granularity of the time is
 *  generally larger than one millisecond and on some systems it may
 *  only be accurate to the second.
 *
 * For some unfathomable reason, the Open Group's "Single Unix Specification"
 *  requires that the ANSI C clock() function be defined in units of 1/1000000
 *  second.  This means that the result overflows a 32-bit signed clock_t
 *  value only about 35 minutes.  Consequently, we use the POSIX standard
 *  times() function instead.
 */

long millisec()
   {
   static long clockres = 0;
   static long starttime = 0;
   long curtime;
   struct tms tp;

   times(&tp);
   curtime = tp.tms_utime + tp.tms_stime;
   if (clockres == 0) {
      #ifdef CLK_TCK
         clockres = CLK_TCK;
      #else
         clockres = sysconf(_SC_CLK_TCK);
      #endif
      starttime = curtime;
      }
   return (long) ((1000.0 / clockres) * (curtime - starttime));
   }
