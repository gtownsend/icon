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

#if MSWIN
   Feature(1, "_MS_WINDOWS", "MS Windows")
#endif					/* MSWIN */

#if CYGWIN
   Feature(1, "_CYGWIN", "Cygwin")
#endif					/* CYGWIN */

#if UNIX
   Feature(1, "_UNIX", "UNIX")
#endif					/* UNIX */

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
