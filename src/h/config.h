/*
 * Icon configuration.
 */

/*
 * System-specific definitions are in define.h, which is loaded first.
 */

/*
 *  A number of symbols are defined here.  Some are specific to individual
 *  to operating systems.  Examples are:
 *
 *	MSDOS		MS-DOS for PCs
 *	UNIX		any UNIX system; also set for BeOS or Mac (Darwin)
 *
 *  There also are definitions of symbols for specific computers and
 *  versions of operating systems.  These include:
 *
 *	MICROSOFT	code specific to the Microsoft C compiler for MS-DOS
 *
 *  Other definitions may occur for different configurations. These include:
 *
 *	DeBug		debugging code
 *	MultiThread	support for multiple programs under the interpreter
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

#ifndef NoStrInvoke
   #undef StrInvoke
   #define StrInvoke
#endif					/* NoStrInvoke */

#ifndef NoLargeInts
   #undef LargeInts
   #define LargeInts
#endif					/* NoLargeInts */

#ifdef EventMon
   #undef MultiThread
   #define MultiThread
#endif					/* EventMon */

#ifndef NoExternalFunctions
   #undef ExternalFunctions
   #define ExternalFunctions
#endif					/* NoExternalFunctions */

/*
 * Names for standard environment variables.
 * The standard names are used unless they are overridden.
 */

#ifndef NOERRBUF
   #define NOERRBUF "NOERRBUF"
#endif

#ifndef TRACE
   #define TRACE "TRACE"
#endif

#ifndef COEXPSIZE
   #define COEXPSIZE "COEXPSIZE"
#endif

#ifndef STRSIZE
   #define STRSIZE "STRSIZE"
#endif

#ifndef HEAPSIZE
   #define HEAPSIZE "HEAPSIZE"
#endif

#ifndef BLOCKSIZE
   #define BLOCKSIZE "BLOCKSIZE"
#endif

#ifndef BLKSIZE
   #define BLKSIZE "BLKSIZE"
#endif

#ifndef MSTKSIZE
   #define MSTKSIZE "MSTKSIZE"
#endif

#ifndef QLSIZE
   #define QLSIZE "QLSIZE"
#endif

#ifndef ICONCORE
   #define ICONCORE "ICONCORE"
#endif

#ifndef IPATH
   #define IPATH "IPATH"
#endif

/*
 * Graphics definitions.
 */

#ifdef MSWindows
   #undef Graphics
   #define Graphics 1
   #ifndef NTConsole
      #define ConsoleWindow 1
   #endif				/* NTConsole */
#endif					/* MSWindows */

#ifdef Graphics
   #ifndef NoXpmFormat
      #if UNIX
         #undef HaveXpmFormat
         #define HaveXpmFormat
      #endif				/* UNIX */
   #endif				/* NoXpmFormat */

   #ifndef MSWindows
      #undef XWindows
      #define XWindows 1
   #endif				/* MSWindows */

   #undef LineCodes
   #define LineCodes

   #undef Polling
   #define Polling

   #ifndef ICONC_XLIB
      #define ICONC_XLIB "-L/usr/X11R6/lib -lX11"
   #endif				/* ICONC_XLIB */

   #ifdef ConsoleWindow
      /*
       * knock out fprintf and putc; these are here so that consoles may be used
       * in icont and rtt, not just iconx
       */
      #undef fprintf
      #define fprintf Consolefprintf
      #undef putc
      #define putc Consoleputc
      #undef fflush
      #define fflush Consolefflush
      #undef printf
      #define printf Consoleprintf
      #undef exit
      #define exit c_exit
   #endif				/* Console Window */

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
   #define MaxHdr 4096
#endif					/* MaxHdr */

#ifndef MaxPath
   #define MaxPath 200
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
 * Features enabled by default under certain systems
 */
#ifndef Pipes
   #if UNIX
      #define Pipes
   #endif				/* UNIX */
#endif					/* Pipes */

#ifndef KeyboardFncs
   #if UNIX
      #define KeyboardFncs
   #endif				/* UNIX */
#endif					/* KeyboardFncs */

#ifndef ReadDirectory
   #if UNIX
      #define ReadDirectory
   #endif				/* UNIX*/
#endif					/* ReadDirectory */

#ifndef SysOpt
   #if UNIX
      #define SysOpt
   #endif				/* UNIX*/
#endif					/* SysOpt */

#ifndef NoWildCards
   #if NT
      #define WildCards 1
   #else				/* NT || ... */
      #define WildCards 0
   #endif				/* NT || ... */
#else					/* NoWildCards */
   #define WildCards 0
#endif					/* NoWildCards */

/*
 *  The following definitions assume ANSI C.
 */
#define Cat(x,y) x##y
#define Lit(x) #x
#define Bell '\a'

/*
 * Customize output if not pre-defined.
 */

#ifndef TraceOut
   #define TraceOut(s) fprintf(stderr,s)
#endif					/* TraceOut */

/*
 * File opening modes.
 */

#ifndef WriteBinary
   #define WriteBinary "wb"
#endif					/* WriteBinary */

#ifndef ReadBinary
   #define ReadBinary "rb"
#endif					/* ReadBinary */

#ifndef ReadWriteBinary
   #define ReadWriteBinary "wb+"
#endif					/* ReadWriteBinary */

#ifndef ReadEndBinary
   #define ReadEndBinary "r+b"
#endif					/* ReadEndBinary */

#ifndef WriteText
   #define WriteText "w"
#endif					/* WriteText */

#ifndef ReadText
   #define ReadText "r"
#endif					/* ReadText */

/*
 * Miscellaney.
 */

#ifndef DiffPtrs
   #define DiffPtrs(p1,p2) (word)((p1)-(p2))
#endif					/* DiffPtrs */

#ifndef AllocReg
   #define AllocReg(n) malloc(n)
#endif					/* AllocReg */

#define MaxFileName 256

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

#ifndef EventMon
   #if IntBits == 16
      #define NoSrcColumnInfo
   #endif				/* IntBits == 16 */
#endif					/* EventMon */

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
   #undef ExecImages		/* interpreter only */

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
 * Executable methodology.
 */

#if UNIX
   #undef Header
   #define Header
   #undef ShellHeader
   #define ShellHeader
#endif					/* UNIX */

#if MSDOS && !NT
   #undef DirectExecution
   #define DirectExecution
#endif					/* MSDOS && !NT */

#ifdef Header
   #undef DirectExecution
   #define DirectExecution
#endif					/* Header */

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
