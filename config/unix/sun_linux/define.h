/*  
 * Icon configuration file for Red Hat Linux v2 on Sparc
 */

#define UNIX 1

#define LoadFunc

/* Sun parameters */
#define SUN
#define ZERODIVIDE

/* CPU architecture */
#define Double
#define StackAlign 8

/* use gcc to compile generated code */
#define CComp "gcc"
#define COpts "-O2 -fomit-frame-pointer"
