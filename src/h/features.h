/*
 * features.h -- predefined symbols and &features
 *
 * This file consists entirely of a sequence of conditionalized calls
 *  to the Feature() macro.  The macro is not defined here, but is
 *  defined to different things by the the code that includes it.
 *
 * For the macro call  Feature(guard,symname,kwval)
 * the parameters are:
 *    guard	for the compiler's runtime system, an expression that must
 *		evaluate as true for the feature to be included in &features
 *    symname	predefined name in the preprocessor; "" if none
 *    kwval	value produced by the &features keyword; 0 if none
 *
 * The translator and compiler modify this list of predefined symbols
 * through calls to ppdef().
 */

   Feature(1, "_V9", 0)			/* Version 9 (unconditional) */

#if AMIGA
   Feature(1, "_AMIGA", "Amiga")
#endif					/* AMIGA */

#if ARM
   Feature(1, "_ACORN", "Acorn Archimedes")
#endif					/* ARM */

#if MACINTOSH
   Feature(1, "_MACINTOSH", "Macintosh")
#endif					/* MACINTOSH */

#if MSDOS
#if INTEL_386 || HIGHC_386 || WATCOM || ZTC_386 || BORLAND_386 || SCCX_MX
   Feature(1, "_MSDOS_386", "MS-DOS/386")
#else					/* INTEL_386 || HIGHC_386 ... */
#if NT
   Feature(1, "_MS_WINDOWS_NT", "MS Windows NT")
#else					/* NT */
#if BORLAND_286
   Feature(1, "_MSDOS_286", "MS-DOS/286")
#else					/* BORLAND_286 */
   Feature(1, "_MSDOS", "MS-DOS")
#endif					/* BORLAND_286 */
#endif					/* NT */
#endif					/* INTEL_386 || HIGHC_386 ... */
#endif					/* MSDOS */

#if OS2
   Feature(1, "_OS2", "OS/2")
#endif					/* OS2 */

#if PORT
   Feature(1, "_PORT", "PORT")
#endif					/* PORT */

#if UNIX
   Feature(1, "_UNIX", "UNIX")
#endif					/* UNIX */

#if VMS
   Feature(1, "_VMS", "VMS")
#endif					/* VMS */

   Feature(1, "_ASCII", "ASCII")

#ifdef Coexpr
   Feature(1, "_CO_EXPRESSIONS", "co-expressions")
#endif					/* Coexpr */

#ifdef LoadFunc
   Feature(1, "_DYNAMIC_LOADING", "dynamic loading")
#endif					/* LoadFunc */

   Feature(1, "", "environment variables")

#ifdef EventMon
   Feature(1, "_EVENT_MONITOR", "event monitoring")
#endif					/* EventMon */

#ifdef ExternalFunctions
   Feature(1, "_EXTERNAL_FUNCTIONS", "external functions")
#endif					/* ExternalFunctions */

#ifdef KeyboardFncs
   Feature(1, "_KEYBOARD_FUNCTIONS", "keyboard functions")
#endif					/* KeyboardFncs */

#ifdef LargeInts
   Feature(largeints, "_LARGE_INTEGERS", "large integers")
#endif					/* LargeInts */

#ifdef MultiThread
   Feature(1, "_MULTITASKING", "multiple programs")
#endif					/* MultiThread */

#ifdef Pipes
   Feature(1, "_PIPES", "pipes")
#endif					/* Pipes */

   Feature(1, "_SYSTEM_FUNCTION", "system function")

#ifdef Graphics
   Feature(1, "_GRAPHICS", "graphics")
#endif					/* Graphics */

#ifdef XWindows
   Feature(1, "_X_WINDOW_SYSTEM", "X Windows")
#endif					/* XWindows */

#ifdef MSWindows
   Feature(1, "_MS_WINDOWS", "MS Windows")
#if NT
   Feature(1, "_WIN32", "Win32")
#else					/* NT */
   Feature(1, "_WIN16", "Win16")
#endif					/* NT */
#endif					/* MSWindows */

#ifdef PresentationManager
   Feature(1, "_PRESENTATION_MGR", "Presentation Manager")
#endif					/* PresentationManager */

#ifdef ArmFncs
   Feature(1, "_ARM_FUNCTIONS", "Archimedes extensions")
#endif					/* ArmFncs */

#ifdef DosFncs
   Feature(1, "_DOS_FUNCTIONS", "MS-DOS extensions")
#endif					/* DosFncs */
