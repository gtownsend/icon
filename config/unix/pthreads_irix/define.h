/*
 * Icon configuration file for SGI Irix using POSIX threads
 */

/* standard Unix and C */
#define UNIX 1
#define CoClean
#define LoadFunc
#define SysOpt

/* CPU architecture */
#define Double
#define StackAlign 8

/* Irix */
#define CComp "c89"
#define COpts "-Wf,-XNd10000"
