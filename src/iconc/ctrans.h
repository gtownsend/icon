/*
 *  Miscellaneous compiler-specific definitions.
 */

#define Iconc

#ifdef strlen
#undef strlen			/* defined in some contexts */
#endif /* strlen */

#define Abs(n) ((n) >= 0 ? (n) : -(n))
#define Max(x,y) ((x)>(y)?(x):(y))

#if !EBCDIC
#define tonum(c)	(isdigit(c) ? (c - '0') : ((c & 037) + 9))
#endif					/* !EBCDIC */

/*
 * Hash tables must be a power of 2.
 */
#define CHSize 128	/* size of constant hash table */
#define FHSize  32	/* size of field hash table */
#define GHSize 128	/* size of global hash table */
#define LHSize 128	/* size of local hash table */

#define PrfxSz 3        /* size of prefix */

/*
 * srcfile is used construct the queue of source files to be translated.
 */
struct srcfile {
   char *name;
   struct srcfile *next;
   };

extern struct srcfile *srclst;

/*
 * External definitions needed throughout translator.
 */
extern int twarns;

#ifdef TranStats
#include "tstats.h"
#else					/* TranStats */
#define TokInc(x)
#define TokDec(x)
#endif					/* TranStats */
