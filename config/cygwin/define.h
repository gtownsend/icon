/*
 * Icon configuration file for Cygwin environment on Microsoft Windows
 */
#define MSWIN 1
#define CYGWIN 1

#define FAttrib

#define CComp "gcc"
#define ExecSuffix ".exe"

/*
 * Header is used in winnt.h; careful how we define it here
 */
#define Header header

/*
 * Uncomment the following line to compile Icon programs to Windows CMD
 * files. Otherwise, Icon programs will have executable headers.
 */
/* #define ShellHeader */

#ifdef ShellHeader
   #define IcodeSuffix ".cmd"
#else					/* ShellHeader */
   #define IcodeSuffix ".exe"
#endif					/* ShellHeader */
