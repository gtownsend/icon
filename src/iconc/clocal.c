/*
 *  clocal.c -- compiler functions needed for different systems.
 */
#include "../h/gsupport.h"

/*
 * The following code is operating-system dependent [@tlocal.01].
 *  Routines needed by different systems.
 */

#if PORT
/* place to put anything system specific */
Deliberate Syntax Error
#endif					/* PORT */

#if MACINTOSH
#if MPW
/*
 * These are stubs for several routines defined in the runtine
 *  library that aren't necessary in MPW tools.  These routines are 
 *  referenced by the Standard C Library I/O functions, but are never called.
 *  Because they are referenced, the linker can't remove them.  The stubs in
 *  this file provide dummy routines which are never called, but reduce the
 *  size of the tool.
 */


/* Console Driver

   These drivers provide I/O to the screen (or a specified port) in
   applications.  They aren't necessary in tools.  
*/

_coFAccess() {}
_coClose() {}
_coRead() {}
_coWrite() {}
_coIoctl() {}
_coExit() {}


/* File System Driver

   Tools use the file system drivers linked with the MPW Shell.
*/

_fsFAccess() {}
_fsClose() {}
_fsRead() {}
_fsWrite() {}
_fsIoctl() {}
_fsExit() {}


/* System Driver

   Tools use the system drivers linked with the MPW Shell.
*/

_syFAccess() {}
_syClose() {}
_syRead() {}
_syWrite() {}
_syIoctl() {}
_syExit() {}


/* Floating Point Conversion Routines

   These routines, called by printf, are only necessary if floating point
   formatting is used.
*/

#endif					/* MPW */
#endif					/* MACINTOSH */

#if MSDOS

#if MICROSOFT

pointer xmalloc(n)
   long n;
   {
   return calloc((size_t)n,sizeof(char));
   }
#endif					/* MICROSOFT */

#if MICROSOFT
int _stack = (8 * 1024);
#endif					/* MICROSOFT */

#if TURBO
extern unsigned _stklen = 8192;
#endif					/* TURBO */

#if ZTC_386
#ifndef DOS386
int _stack = (8 * 1024);
#endif					/* DOS386 */
#endif					/* ZTC_386 */

#endif					/* MSDOS */

#if MVS || VM
#endif					/* MVS || VM */

#if OS2
#endif					/* OS2 */

#if UNIX
#endif					/* UNIX */

#if VMS
#endif					/* VMS */

/*
 * End of operating-system specific code.
 */

char *tjunk;			/* avoid empty module */
