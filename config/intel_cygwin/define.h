/*
 * Icon configuration file for Cygwin environment on Microsoft Windows
 */
#define MSWIN 1		/* this configuration is for Microsoft Windows */
#define CYGWIN 1	/* this configuration uses Cygwin API */

#define FAttrib		/* enable fattrib() extension */
#define WinExtns	/* enable native Windows functions */

#define CComp "gcc"
#define ExecSuffix ".exe"

/*
 * Comment out the follwing line to compile Icon programs to Windows CMD
 * files. Otherwise, Icon programs will have executable binary headers.
 */
#define BinaryHeader

#ifdef BinaryHeader
   #define IcodeSuffix ".exe"
#else					/* BinaryHeader */
   #define IcodeSuffix ".cmd"
#endif					/* BinaryHeader */
