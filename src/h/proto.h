/*
 * proto.h -- prototypes for library functions.
 */

#if MSDOS
   #if MICROSOFT || NT
      #include <dos.h>
   #endif				/* MICROSOFT || NT */
#endif					/* MSDOS */

#include "../h/mproto.h"

/*
 * These must be after prototypes to avoid clash with system
 * prototypes.
 */

#if IntBits == 16
   #define sbrk lsbrk
   #define strlen lstrlen
   #define qsort lqsort
#endif					/* IntBits == 16 */
