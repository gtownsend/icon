/*
 * Icon configuration file for Sun 4 running Solaris 2.x with Sun cc
 */

#define UNIX 1
#define SUN

#define LoadFunc
#define SysOpt
#define NoRanlib

/* CPU architecture */
#define Double
#define StackAlign 8

#define CComp "cc"
#define COpts "-I/usr/openwin/include -ldl"
