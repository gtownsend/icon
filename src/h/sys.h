/*
 * sys.h -- system include files.
 */

/*
 * Universal (Standard 1989 ANSI C) includes.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * POSIX (1003.1-1996) includes.
 */
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/utsname.h>

/*
 * Operating-system-dependent includes.
 */
#if MSWIN
   #include <windows.h>
   #include <sys/cygwin.h>
   #include <sys/select.h>
   
   #ifdef WinGraphics
      #define int_PASCAL int PASCAL
      #define LRESULT_CALLBACK LRESULT CALLBACK
      #define BOOL_CALLBACK BOOL CALLBACK
      #include <mmsystem.h>
      #include <process.h>
      #include "../wincap/dibutil.h"
   #endif				/* WinGraphics */

   #undef Type
   #undef lst1
   #undef lst2
#endif					/* MSWIN */

/*
 * Window-system-dependent includes.
 */
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

/*
 * Feature-dependent includes.
 */
#ifdef LoadFunc
   #include <dlfcn.h>
#endif					/* LoadFunc */
