/*
 * Icon configuration file for NetBSD
 */

#define UNIX 1
#define GenericBSD
#define BSD_4_4_LITE    1	/* This is new, for 4.4Lite specific stuff */
#define NetBSD			/* This is for NetBSD stuff (save) */

#define LoadFunc
#define SysOpt
#define ExecImages

#define MaxStatSize 20480

#define CComp "gcc"
#define COpts "-O2 -I /usr/X11R6/include"
#define LinkLibs " -lm"

