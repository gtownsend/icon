/*  
 * Icon configuration file for Linux using POSIX threads
 */

/* standard Unix and C */
#define UNIX 1
#define CoClean
#define LoadFunc
/* no SysOpt: GNU getopt() is POSIX-compatible only if an envmt var is set */

/* GNU/Linux */
#define CComp "gcc"
#define COpts "-O2 -fomit-frame-pointer"
