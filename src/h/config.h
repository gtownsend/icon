/*
 * Icon configuration.
 */

/*
 * System-specific definitions are in define.h, which is loaded first.
 */

/*
 *  A number of symbols are defined here.
 *  Some enable or disable certain Icon features, for example:
 *	NoCoexpr	disables co-expressions
 *	LoadFunc	enables dynamic loading
 *
 *  Other definitions may occur for different configurations. These include:
 *	DeBug		debugging code
 *	MultiThread	support for multiple programs under the interpreter
 *
 *  Many definitions reflect remnants of past research projects.
 *  Changing them to values not used in standard configurations
 *  may result in an unbuildable or nonfunctioning system.
 */

/*
 * If COMPILER is not defined, code for the interpreter is compiled.
 */

#ifndef COMPILER
   #define COMPILER 0
#endif

/*
 * The following definitions serve to cast common conditionals is
 *  a positive way, while allowing defaults for the cases that
 *  occur most frequently.  That is, if co-expressions are not supported,
 *  NoCoexpr is defined in define.h, but if they are supported, no
 *  definition is needed in define.h; nonetheless subsequent conditionals
 *  can be cast as #ifdef Coexpr.
 */

#ifndef NoCoexpr
   #undef Coexpr
   #define Coexpr
#endif					/* NoCoexpr */

#ifdef NoCoexpr
   #undef MultiThread
   #undef EventMon
   #undef Eve
#endif					/* NoCoexpr */

#if COMPILER
   #undef Eve
   #undef MultiThread
   #undef EventMon
#endif					/* COMPILER */

#ifdef Eve
   #undef EventMon
   #undef MultiThread
   #define EventMon
   #define MultiThread
#endif					/* Eve */

#ifndef NoLargeInts
   #undef LargeInts
   #define LargeInts
#endif					/* NoLargeInts */

#ifdef EventMon
   #undef MultiThread
   #define MultiThread
#endif					/* EventMon */

/*
 * Graphics definitions.
 */
#ifdef Graphics

   #ifndef XWindows
      #ifdef MSWIN
         #undef WinGraphics
         #define WinGraphics 1
      #else				/* Graphics */
         #define XWindows 1
      #endif				/* Graphics */
   #endif				/* XWindows */

   #ifndef NoXpmFormat
      #ifdef XWindows
         #undef HaveXpmFormat
         #define HaveXpmFormat
      #endif				/* XWindows */
   #endif				/* NoXpmFormat */

   #undef LineCodes
   #define LineCodes

   #undef Polling
   #define Polling

   #ifndef ICONC_XLIB
      #ifdef WinGraphics
         #define ICONC_XLIB "-luser32 -lgdi32 -lcomdlg32 -lwinmm"
      #else				/* WinGraphics */
         #define ICONC_XLIB "-L/usr/X11R6/lib -lX11"
      #endif				/* WinGraphics */
   #endif				/* ICONC_XLIB */

#endif					/* Graphics */

/*
 * Data sizes and alignment.
 */

#define WordSize sizeof(word)

#ifndef StackAlign
   #define StackAlign 8
#endif					/* StackAlign */

/*
 * Other defaults.
 */

#ifdef DeBug
   #undef DeBugTrans
   #undef DeBugLinker
   #undef DeBugIconx
   #define DeBugTrans
   #define DeBugLinker
   #define DeBugIconx
#endif					/* DeBug */

#ifndef MaxHdr
   /*
    * Maximum allowable BinaryHeader size.
    * WARNING: changing this invalidates old BinaryHeader executables.
    */
   #define MaxHdr 8192
#endif					/* MaxHdr */

#ifndef MaxPath
   #define MaxPath 256
#endif					/* MaxPath */

#ifndef SourceSuffix
   #define SourceSuffix ".icn"
#endif					/* SourceSuffix */

/*
 * Representations of directories. LocalDir is the "current working directory".
 *  SourceDir is where the source file is.
 */

#define LocalDir ""
#define SourceDir (char *)NULL

#ifndef TargetDir
   #define TargetDir LocalDir
#endif					/* TargetDir */

/*
 * Features enabled by default.
 */
#ifndef NoPipes
   #define Pipes
#endif					/* Pipes */

#ifndef NoKeyboardFncs
   #define KeyboardFncs
#endif					/* KeyboardFncs */

#ifndef NoReadDirectory
   #define ReadDirectory
#endif					/* ReadDirectory */

#ifndef NoSysOpt
   #define SysOpt
#endif					/* SysOpt */

/*
 *  The following definitions assume ANSI C.
 */
#define Cat(x,y) x##y
#define Lit(x) #x
#define Bell '\a'

/*
 * Miscellaney.
 */

#ifndef DiffPtrs
   #define DiffPtrs(p1,p2) (word)((p1)-(p2))
#endif					/* DiffPtrs */

#ifndef AllocReg
   #define AllocReg(n) malloc(n)
#endif					/* AllocReg */

#ifndef RttSuffix
   #define RttSuffix ".r"
#endif					/* RttSuffix */

#ifndef DBSuffix
   #define DBSuffix ".db"
#endif					/* DBSuffix */

#ifndef PPInit
   #define PPInit ""
#endif					/* PPInit */

#ifndef PPDirectives
   #define PPDirectives {"passthru", PpKeep},
#endif					/* PPDirectives */

#ifndef NoSrcColumnInfo
   #define SrcColumnInfo
#endif					/* NoSrcColumnInfo */

#ifndef ExecSuffix
   #define ExecSuffix ""
#endif					/* ExecSuffix */

#ifndef CSuffix
   #define CSuffix ".c"
#endif					/* CSuffix */

#ifndef HSuffix
   #define HSuffix ".h"
#endif					/* HSuffix */

#ifndef ObjSuffix
   #define ObjSuffix ".o"
#endif					/* ObjSuffix */

#ifndef LibSuffix
   #define LibSuffix ".a"
#endif					/* LibSuffix */

#ifndef CComp
   #define CComp "cc"
#endif					/* CComp */

#ifndef COpts
   #define COpts ""
#endif					/* COpts */

/*
 * Note, size of the hash table is a power of 2:
 */
#define IHSize 128
#define IHasher(x)	(((unsigned int)(unsigned long)(x))&(IHSize-1))

#if COMPILER

   /*
    * Code for the compiler.
    */
   #undef MultiThread		/* no way -- interpreter only */
   #undef EventMon		/* presently not supported in the compiler */

#else					/* COMPILER */

   /*
    * Code for the interpreter.
    */
   #ifndef IcodeSuffix
      #define IcodeSuffix ""
   #endif				/* IcodeSuffix */

   #ifndef IcodeASuffix
      #define IcodeASuffix ""
   #endif				/* IcodeASuffix */

   #ifndef U1Suffix
      #define U1Suffix ".u1"
   #endif				/* U1Suffix */

   #ifndef U2Suffix
      #define U2Suffix ".u2"
   #endif				/* U2Suffix */

   #ifndef USuffix
      #define USuffix ".u"
   #endif				/* USuffix */

#endif					/* COMPILER */

/*
 *  Vsizeof is for use with variable-sized (i.e., indefinite)
 *   structures containing an array of descriptors declared of size 1
 *   to avoid compiler warnings associated with 0-sized arrays.
 */

#define Vsizeof(s)	(sizeof(s) - sizeof(struct descrip))

/*
 * Other sizeof macros:
 *
 *  Wsizeof(x)	-- Size of x in words.
 *  Vwsizeof(x) -- Size of x in words, minus the size of a descriptor.	Used
 *   when structures have a potentially null list of descriptors
 *   at their end.
 */

#define Wsizeof(x)	((sizeof(x) + sizeof(word) - 1) / sizeof(word))
#define Vwsizeof(x) \
   ((sizeof(x) - sizeof(struct descrip) + sizeof(word) - 1) / sizeof(word))
