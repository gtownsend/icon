/*
 * Icon configuration file for Sun 4 running Solaris 2.x with CenterLine C
 */

#define UNIX 1
#define SUN

#define LoadFunc
#define SysOpt
#define NoRanlib

/* CPU architecture */
#define Double
#define StackAlign 8

/* use clcc to compile generated code */
#define CComp "clcc"
#define COpts "-w -I/usr/openwin/include -ldl -R/usr/openwin/lib"
