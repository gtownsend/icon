/*
 * Icon configuration file for FreeBSD on Alpha
 */

#define UNIX 1
#define GenericBSD
#define BSD_4_4_LITE    1	/* This is new, for 4.4Lite specific stuff */

#define LoadFunc
#define SysOpt
#define ExecImages

#define IntBits 32
#define WordBits 64
#define Double
#define StackAlign 8

#define MaxStatSize 20480

#define CComp "gcc"
#define COpts "-O2"
