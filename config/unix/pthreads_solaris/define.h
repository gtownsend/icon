/*  
 * Icon configuration file for Solaris using POSIX threads 
 */

/* standard Unix and C */
#define UNIX 1
#define CoClean
#define LoadFunc
#define SysOpt

/* Solaris and Sun C */
#define SUN
#define CComp "c89"
#define COpts "-I/usr/openwin/include -ldl"
