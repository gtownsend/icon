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
#if PORT
   Deliberate Syntax Error
#endif					/* PORT */

#if AMIGA
   #include <fcntl.h>
   #include <ios1.h>
   #include <libraries/dosextens.h>
   #include <libraries/dos.h>
   #include <workbench/startup.h>
   #if __SASC
      #include <proto/dos.h>
      #include <proto/icon.h>
      #include <proto/wb.h>
      #undef GLOBAL
      #undef STATIC			/* defined in <exec/types.h> */
   #endif				/* __SASC */
#endif					/* AMIGA */

#if ATARI_ST
   #include <fcntl.h>
   #include <osbind.h>
#endif					/* ATARI_ST */

#if MACINTOSH
   #if LSC
      #include <unix.h>
   #endif				/* LSC */
   #if MPW
      #define create xx_create	/* prevent duplicate definition of create() */
      #include <Types.h>
      #include <Events.h>
      #include <Files.h>
      #include <FCntl.h>
      #include <Files.h>
      #include <IOCtl.h>
      #include <fp.h>
      #include <OSUtils.h>
      #include <Memory.h>
      #include <Errors.h>
      #include "time.h"
      #include <Quickdraw.h>
      #include <ToolUtils.h>
      #include <CursorCtl.h>
   #endif				/* MPW */
   #ifdef MacGraph
      #include <console.h>
      #include <AppleEvents.h>
      #include <GestaltEqu.h>
      #include <fp.h>
      #include <QDOffscreen.h>
      #include <Palettes.h>
      #include <Quickdraw.h>
   #endif				/* MacGraph */
#endif					/* MACINTOSH */

#if MSDOS
   #undef Type
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>

   #ifdef MSWindows
      #define int_PASCAL int PASCAL
      #define LRESULT_CALLBACK LRESULT CALLBACK
      #define BOOL_CALLBACK BOOL CALLBACK
      #if BORLAND_286
         #include <dir.h>
      #endif				/* BORLAND_286 */
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

#if MVS || VM
   #ifdef RecordIO
      #if SASC
         #include <lcio.h>
      #endif				/* SASC */
   #endif				/* RecordIO */
   #if SASC
      #include <lcsignal.h>
   #endif				/* SASC */
#endif					/* MVS || VM */

#if OS2
   #define INCL_DOS
   #define INCL_ERRORS
   #define INCL_RESOURCES
   #define INCL_DOSMODULEMGR

   #ifdef PresentationManager
      #define INCL_PM
   #endif				/* PresentationManager */

   #include <os2.h>
   /* Pipe support for OS/2 */
   #include <stddef.h>
   #include <process.h>
   #include <fcntl.h>

   #if CSET2V2
      #include <io.h>
      #include <direct.h>
      #define IN_SYS_H
      #include "../h/local.h"		/* Include #pragmas */
      #undef IN_SYS_H
   #endif				/* CSet/2 version 2 */

#endif					/* OS2 */

#if UNIX
   #include <dirent.h>
   #include <limits.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/time.h>
   #include <sys/times.h>
   #include <sys/types.h>
   #include <termios.h>
   #ifdef SysSelectH
      #include <sys/select.h>
   #endif
#endif					/* UNIX */

#if VMS
   #include <types.h>
   #include <dvidef>
   #include <iodef>
   #include <stsdef.h>
#endif					/* VMS */

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
   /*
    * Undef VMS under UNIX, and UNIX under VMS,
    * to avoid confusing the tests within the X header files.
    */
   #if VMS
      #undef UNIX
      #include "decw$include:Xlib.h"
      #include "decw$include:Xutil.h"
      #include "decw$include:Xos.h"
      #include "decw$include:Xatom.h"

      #ifdef HaveXpmFormat
         #include "../xpm/xpm.h"
      #endif				/* HaveXpmFormat */

      #undef UNIX
      #define UNIX 0
   #else				/* VMS */
      #undef VMS

      #ifdef HaveXpmFormat
         #include "../xpm/xpm.h"
      #else				/* HaveXpmFormat */
         #include <X11/Xlib.h>
      #endif				/* HaveXpmFormat */

      #include <X11/Xutil.h>
      #include <X11/Xos.h>
      #include <X11/Xatom.h>

      #undef VMS
      #define VMS 0
   #endif				/* VMS */
#endif					/* XWindows */

#ifdef Graphics
   #define VanquishReturn(s) return s;
#endif					/* Graphics */

/*
 * Feature-dependent includes.
 */
#ifndef HostStr
   #if !VMS
      #include <sys/utsname.h>
   #endif				/* !VMS */
#endif					/* HostStr */

#ifdef LoadFunc
   #include <dlfcn.h>
#endif					/* LoadFunc */

#if WildCards
   #include "../h/filepat.h"
#endif					/* WildCards */
