/*
 * tmem.c -- memory initialization and allocation for the translator.
 */

#include "../h/gsupport.h"
#include "tproto.h"
#include "tglobals.h"
#include "tsym.h"
#include "tree.h"

struct tlentry **lhash;		/* hash area for local table */
struct tgentry **ghash;		/* hash area for global table */
struct tcentry **chash;		/* hash area for constant table */

struct tlentry *lfirst;		/* first local table entry */
struct tlentry *llast;		/* last local table entry */
struct tcentry *cfirst;		/* first constant table entry */
struct tcentry *clast;		/* last constant table entry */
struct tgentry *gfirst;		/* first global table entry */
struct tgentry *glast;		/* last global table entry */

extern struct str_buf lex_sbuf;


/*
 * tmalloc - allocate memory for the translator
 */

void tmalloc()
{
   chash = (struct tcentry **) tcalloc(lchsize, sizeof (struct tcentry *));
   ghash = (struct tgentry **) tcalloc(ghsize, sizeof (struct tgentry *));
   lhash = (struct tlentry **) tcalloc(lhsize, sizeof (struct tlentry *));
   init_str();
   init_sbuf(&lex_sbuf);
   }

/*
 * meminit - clear tables for use in translating the next file
 */
void tminit()
   {
   register struct tlentry **lp;
   register struct tgentry **gp;
   register struct tcentry **cp;

   lfirst = NULL;
   llast = NULL;
   cfirst = NULL;
   clast = NULL;
   gfirst = NULL;
   glast = NULL;

   /*
    * Zero out the hash tables.
    */
   for (lp = lhash; lp < &lhash[lhsize]; lp++)
      *lp = NULL;
   for (gp = ghash; gp < &ghash[ghsize]; gp++)
      *gp = NULL;
   for (cp = chash; cp < &chash[lchsize]; cp++)
      *cp = NULL;
   }

/*
 * tmfree - free memory used by the translator
 */
void tmfree()
   {
   free((char *) chash);   chash = NULL;
   free((char *) ghash);   ghash = NULL;
   free((char *) lhash);   lhash = NULL;

   free_stbl();           /* free string table */
   clear_sbuf(&lex_sbuf); /* free buffer store for strings */
   }
