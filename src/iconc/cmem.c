/*
 * cmem.c -- memory initialization and allocation for the translator.
 */
#include "../h/gsupport.h"
#include "cglobals.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ccode.h"
#include "cproto.h"

struct centry *chash[CHSize];		/* hash area for constant table */
struct fentry *fhash[FHSize];		/* hash area for field table */
struct gentry *ghash[GHSize];		/* hash area for global table */

struct implement *bhash[IHSize];	/* hash area for built-in functions */
struct implement *khash[IHSize];	/* hash area for keywords */
struct implement *ohash[IHSize];	/* hash area for operators */

struct implement *spec_op[NumSpecOp];	/* table of ops with special syntax */

char pre[PrfxSz] = {'0', '0', '0'};     /* initial function name prefix */

extern struct str_buf lex_sbuf;


/*
 * init - initialize memory for the translator
 */

void init()
{
   int i;

   init_str();
   init_sbuf(&lex_sbuf);

   /*
    * Zero out the hash tables.
    */
   for (i = 0; i < CHSize; i++)
      chash[i] = NULL;
   for (i = 0; i < FHSize; i++)
      fhash[i] = NULL;
   for (i = 0; i < GHSize; i++)
      ghash[i] = NULL;
   for (i = 0; i < IHSize; i++) {
      bhash[i] = NULL;
      khash[i] = NULL;
      ohash[i] = NULL;
      }

   /*
    * Clear table of operators with non-standard operator syntax.
    */
   for (i = 0; i < NumSpecOp; ++i)
      spec_op[i] = NULL;
   }

/*
 * init_proc - add a new entry on front of procedure list.
 */
void init_proc(name)
char *name;
   {
   register struct pentry *p;
   int i;
   struct gentry *sym_ent;

   p = NewStruct(pentry);
   p->name = name;
   nxt_pre(p->prefix, pre, PrfxSz);
   p->prefix[PrfxSz] = '\0';
   p->nargs = 0;
   p->args = NULL;
   p->ndynam = 0;
   p->dynams = NULL;
   p->nstatic = 0;
   p->has_coexpr = 0;
   p->statics = NULL;
   p->ret_flag = DoesRet | DoesFail | DoesSusp; /* start out pessimistic */
   p->arg_lst = 0;
   p->lhash = 
     (struct lentry **)alloc((unsigned int)((LHSize)*sizeof(struct lentry *)));
   for (i = 0; i < LHSize; i++)
      p->lhash[i] = NULL;
   p->next = proc_lst;
   proc_lst = p;
   sym_ent = instl_p(name, F_Proc);
   sym_ent->val.proc = proc_lst;
   }

/*
 * init_rec - add a new entry on the front of the record list.
 */
void init_rec(name)
char *name;
   {
   register struct rentry *r;
   struct gentry *sym_ent;
   static int rec_num = 0;

   r = NewStruct(rentry);
   r->name = name;
   nxt_pre(r->prefix, pre, PrfxSz);
   r->prefix[PrfxSz] = '\0';
   r->rec_num = rec_num++;
   r->nfields = 0;
   r->fields = NULL;
   r->next = rec_lst;
   rec_lst = r;
   sym_ent= instl_p(name, F_Record);
   sym_ent->val.rec = r;
   }
