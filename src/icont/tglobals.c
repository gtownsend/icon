/*
 * tglobals.c - declaration and initialization of icont globals.
 */

#include "../h/gsupport.h"
#include "tproto.h"

#define Global
#define Init(v) = v
#include "tglobals.h"			/* define globals */

/*
 *  Initialize globals that cannot be handled statically.
 */
void initglob(void) {
   /*
    * Round hash table sizes to next power of two, and set masks for hashing.
    */
   lchsize = round2(lchsize);  cmask = lchsize - 1;
   fhsize = round2(fhsize);  fmask = fhsize - 1;
   ghsize = round2(ghsize);  gmask = ghsize - 1;
   ihsize = round2(ihsize);  imask = ihsize - 1;
   lhsize = round2(lhsize);  lmask = lhsize - 1;
   }
