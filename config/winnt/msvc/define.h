#define HostStr "Win32 Intel"
#define IconAlloc
#define Precision 16
#define IcodeSuffix ".exe"
#define IcodeASuffix ".EXE"
#define SystemFnc
#define SysTime <sys/time.h>
#define Standard
/*
 * Header is used in winnt.h; careful how we define it here
 */
#define Header header
#define NT 1
#define ShellHeader

#define index strchr
#define rindex strrchr
#define strdup _strdup
#define unlink _unlink

#define MSDOS 1
#define StandardC
#define Double
#define ZERODIVIDE
/* #define Eve */
