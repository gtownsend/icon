/* test nested conditionals */

#if 1
#if 0
1
#else
2
#endif
#else
#if 1
3
#else
4
#endif
#endif

#if 0
   #if 0
   5
   #else
   6
   #endif
#else
   #if 1
   7
   #else
   8
   #endif
#endif

#ifndef x
   #ifdef y
   9
   #elif 1
   10
   #else
   11
   #endif
#elif 1
   12
#else
   13
#endif
   
#define x

#ifndef x
   #ifdef y
   14
   #elif 1
   15
   #else
   16
   #endif
#elif 1
   17
#else
   18
#endif

#if 1
   #if 1
      #if 1
      19
      #else
      20
      #endif
   #elif 1
      21
   #endif
#endif

#if 0
   #ifndef y
      #ifdef x
      22
      #elif 1
      23
      #elif 1
      24
      #endif
   #else
   25
   #endif
#elif 1
26
#endif
