/*
 * sys.h -- system include files.
 */

/*
 * Universal (Standard ANSI C) includes.
 */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Operating-system-dependent includes.
 */
#if MSDOS
   #undef Type
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>

   #ifdef MSWindows
      #define int_PASCAL int PASCAL
      #define LRESULT_CALLBACK LRESULT CALLBACK
      #define BOOL_CALLBACK BOOL CALLBACK
      #include <windows.h>
      #include <mmsystem.h>
      #include <process.h>
      #include "../wincap/dibutil.h"
   #endif				/* MSWindows */
   #include <setjmp.h>
   #define Type(d) (int)((d).dword & TypeMask)
   #undef lst1
   #undef lst2
#endif					/* MSDOS */

#if UNIX
   #include <dirent.h>
   #include <limits.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/time.h>
   #include <sys/times.h>
   #include <sys/types.h>
   #include <termios.h>
#endif					/* UNIX */

/*
 * Window-system-dependent includes.
 */
#ifdef ConsoleWindow
   #include <stdarg.h>
   #undef printf
   #undef fprintf
   #undef fflush
   #define printf Consoleprintf
   #define fprintf Consolefprintf
   #define fflush Consolefflush
#endif					/* ConsoleWindow */

#ifdef XWindows
   #ifdef HaveXpmFormat
      #include "../xpm/xpm.h"
   #else				/* HaveXpmFormat */
      #include <X11/Xlib.h>
   #endif				/* HaveXpmFormat */
   #include <X11/Xutil.h>
   #include <X11/Xos.h>
   #include <X11/Xatom.h>
#endif					/* XWindows */

#ifdef Graphics
   #define VanquishReturn(s) return s;
#endif					/* Graphics */

/*
 * Feature-dependent includes.
 */
#ifndef HostStr
   #include <sys/utsname.h>
#endif					/* HostStr */

#ifdef LoadFunc
   #include <dlfcn.h>
#endif					/* LoadFunc */

#if WildCards
   #include "../h/filepat.h"
#endif					/* WildCards */
