/*
 * Group of include files for translators, etc. 
 */

#include "../h/define.h"

#if CSET2V2
   #include <io.h>
#endif					/* CSet/2 ver 2 */

#if !VMS	 /* don't need path.h */
   #include "../h/path.h"
#endif					/* VMS */

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
