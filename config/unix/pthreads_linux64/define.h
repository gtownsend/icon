/*  
 * Icon configuration file for Linux using POSIX threads
 */

/* standard Unix and C */
#define UNIX 1
#define CoClean
#define LoadFunc
/* no SysOpt: GNU getopt() is POSIX-compatible only if an envmt var is set */

/* CPU architecture */
#define IntBits 32
#define WordBits 64
#define Double
#define StackAlign 8

/* GNU/Linux */
#define CComp "gcc"
#define COpts "-O2 -fomit-frame-pointer"
