/*
 * Icon configuration file for Silicon Graphics Irix
 */
 
#define UNIX 1
#define LoadFunc
#define SysOpt

#define IRIS4D
#define CStateSize 32		/* anything >= 26 should actually do */

#ifdef NoCoexpr
   #define MaxStatSize 9000
#endif				/* NoCoexpr */

#define COpts "-Wf,-XNd10000"

#define GammaCorrection 1.0	/* for old X11R5 systems */
