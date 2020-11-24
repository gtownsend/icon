/*
 * Icon configuration.
 */

/*
 * System-specific definitions are in define.h, which is loaded first.
 */

/*
 *  A number of symbols are defined here.
 *  Some enable or disable certain Icon features, for example:
 *	LoadFunc	enables dynamic loading
 *
 *  Many definitions reflect remnants of past research projects.
 *  Changing them to values not used in standard configurations
 *  may result in an unbuildable or nonfunctioning system.
 */

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

#else					/* Graphics */
   #undef XWindows
   #undef WinGraphics
#endif					/* Graphics */

/*
 * Data sizes and alignment.
 */

#define WordSize sizeof(word)

/*
 * Other defaults.
 */

#ifndef MaxHdr
   /*
    * Maximum allowable BinaryHeader size.
    * WARNING: changing this invalidates old BinaryHeader executables.
    */
   #define MaxHdr 8192
#endif					/* MaxHdr */

#ifndef MaxPath
   #define MaxPath 512
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

#ifndef PPDirectives
   #define PPDirectives {"passthru", PpKeep},
#endif					/* PPDirectives */

#ifndef ExecSuffix
   #define ExecSuffix ""
#endif					/* ExecSuffix */

#ifndef CSuffix
   #define CSuffix ".c"
#endif					/* CSuffix */

/*
 * Note, size of the hash table is a power of 2:
 */
#define IHSize 128
#define IHasher(x)	(((unsigned int)(unsigned long)(x))&(IHSize-1))

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
