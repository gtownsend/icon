/*
 * Group of include files for translators, etc.
 */

#include "../h/define.h"

#if !UNIX		 /* don't need path.h */
   #include "../h/path.h"
#endif					/* !UNIX */

#include "../h/arch.h"
#include "../h/config.h"
#include "../h/sys.h"
#include "../h/typedefs.h"
#include "../h/cstructs.h"
#include "../h/proto.h"
#include "../h/cpuconf.h"

#ifdef ConsoleWindow
   #include "../h/rmacros.h"
   #include "../h/rstructs.h"
   #include "../h/graphics.h"
   #include "../h/rexterns.h"
   #include "../h/rproto.h"
#endif					/* ConsoleWindow */
