/*
 * csym.c -- functions for symbol table management.
 */
#include "../h/gsupport.h"
#include "cglobals.h"
#include "ctrans.h"
#include "ctree.h"
#include "ctoken.h"
#include "csym.h"
#include "ccode.h"
#include "cproto.h"

/*
 * Prototypes.
 */

static struct gentry    *alcglob  (struct gentry *blink,
				    char *name,int flag);
static struct fentry    *alcfld   (struct fentry *blink, char *name,
				    struct par_rec *rp);
static struct centry    *alclit	  (struct centry *blink,
				    char *image, int len,int flag);
static struct lentry    *alcloc	  (struct lentry *blink,
				    char *name,int flag);
static struct par_rec   *alcprec  (struct rentry *rec, int offset,
				    struct par_rec *next);
static struct centry    *clookup  (char *image,int flag);
static struct lentry    *dcl_loc  (char *id, int id_type,
                                    struct lentry *next);
static struct lentry    *llookup  (char *id);
static void           opstrinv (struct implement *ip);
static struct gentry    *putglob  (char *id,int id_type);
static struct gentry    *try_gbl  (char *id);

int max_sym = 0;  /* max number of parameter symbols in run-time routines */
int max_prm = 0;  /* max number of parameters for any invocable routine */

/*
 * The operands of the invocable declaration are stored in a list for
 *  later processing.
 */
struct strinv {
   nodeptr op;
   int arity;
   struct strinv *next;
   };
struct strinv *strinvlst = NULL;
int op_tbl_sz;

struct pentry *proc_lst = NULL; /* procedure list */
struct rentry *rec_lst = NULL;  /* record list */


/*
 *instl_p - install procedure or record in global symbol table, returning
 *  the symbol table entry.
 */
struct gentry *instl_p(name, flag)
char *name;
int flag;
   {
   struct gentry *gp;

   flag |= F_Global;
   if ((gp = glookup(name)) == NULL) 
      gp = putglob(name, flag);
   else if ((gp->flag & (~F_Global)) == 0) {
      /* 
       * superfluous global declaration for record or proc
       */
      gp->flag |= flag;
      }
   else			/* the user can't make up his mind */
      tfatal("inconsistent redeclaration", name);
   return gp;
   }

/*
 * install - put an identifier into the global or local symbol table.
 *  The basic idea here is to look in the right table and install
 *  the identifier if it isn't already there.  Some semantic checks
 *  are performed.
 */
void install(name, flag)
char *name;
int flag;
   {
   struct fentry *fp;
   struct gentry *gp;
   struct lentry *lp;
   struct par_rec **rpp;
   struct fldname *fnp;
   int foffset;

   switch (flag) {
      case F_Global:	/* a variable in a global declaration */
         if ((gp = glookup(name)) == NULL)
            putglob(name, flag);
         else
            gp->flag |= flag;
         break;

      case F_Static:	/* static declaration */
         ++proc_lst->nstatic;
         lp = dcl_loc(name, flag, proc_lst->statics);
         proc_lst->statics = lp;
         break;

      case F_Dynamic:	/* local declaration */
         ++proc_lst->ndynam;
         lp = dcl_loc(name, flag, proc_lst->dynams);
         proc_lst->dynams = lp;
         break;

      case F_Argument:	/* formal parameter */
         ++proc_lst->nargs;
         if (proc_lst->nargs > max_prm)
            max_prm = proc_lst->nargs;
         lp = dcl_loc(name, flag, proc_lst->args);
         proc_lst->args = lp;
         break;

      case F_Field: /* field declaration */
         fnp = NewStruct(fldname);
         fnp->name = name;
         fnp->next = rec_lst->fields;
         rec_lst->fields = fnp;
         foffset = rec_lst->nfields++;
         if (foffset > max_prm)
            max_prm = foffset;
         if ((fp = flookup(name)) == NULL) {
            /*
             * first occurrence of this field name.
             */
            fhash[FHasher(name)] = alcfld(fhash[FHasher(name)], name,
               alcprec(rec_lst, foffset, NULL));
            }
         else {
            rpp = &(fp->rlist);
            while (*rpp != NULL && (*rpp)->offset <= foffset &&
               (*rpp)->rec != rec_lst)
               rpp = &((*rpp)->next);
            if (*rpp == NULL || (*rpp)->offset > foffset)
               *rpp = alcprec(rec_lst, foffset, *rpp);
            else
               tfatal("duplicate field name", name);
            }
         break;

      default:
         tsyserr("install: unrecognized symbol table flag.");
      }
   }

/*
 * dcl_loc - handle declaration of a local identifier.
 */
static struct lentry *dcl_loc(name, flag, next)
char *name;
int flag;
struct lentry *next;
   {
   register struct lentry *lp;

   if ((lp = llookup(name)) == NULL) {
      lp = putloc(name,flag);
      lp->next = next;
      }
   else if (lp->flag == flag) /* previously declared as same type */
      twarn("redeclared identifier", name);
   else		/* previously declared as different type */
      tfatal("inconsistent redeclaration", name);
   return lp;
   }

/*
 * putloc - make a local symbol table entry and return pointer to it.
 */
struct lentry *putloc(id,id_type)
char *id;
int id_type;
   {
   register struct lentry *ptr;
   register struct lentry **lhash;
   unsigned hashval;

   if ((ptr = llookup(id)) == NULL) {	/* add to head of hash chain */
      lhash = proc_lst->lhash;
      hashval = LHasher(id);
      ptr = alcloc(lhash[hashval], id, id_type);
      lhash[hashval] = ptr;
      ptr->next = NULL;
      }
   return ptr;
   }

/*
 * putglob makes a global symbol table entry and returns a pointer to it.
 */
static struct gentry *putglob(id, id_type)
char *id;
int id_type;
   {
   register struct gentry *ptr;
   register unsigned hashval;

   if ((ptr = glookup(id)) == NULL) {	 /* add to head of hash chain */
      hashval = GHasher(id);
      ptr = alcglob(ghash[hashval], id, id_type);
      ghash[hashval] = ptr;
      }
   return ptr;
   }

/*
 * putlit makes a constant symbol table entry and returns a pointer to it.
 */
struct centry *putlit(image, littype, len)
char *image;
int len, littype;
   {
   register struct centry *ptr;
   register unsigned hashval;

   if ((ptr = clookup(image,littype)) == NULL) { /* add to head of hash chain */
      hashval = CHasher(image);
      ptr = alclit(chash[hashval], image, len, littype);
      chash[hashval] = ptr;
      }
   return ptr;
   }

/*
 * llookup looks up id in local symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */

static struct lentry *llookup(id)
char *id;
   {
   register struct lentry *ptr;

   ptr = proc_lst->lhash[LHasher(id)];
   while (ptr != NULL && ptr->name != id)
      ptr = ptr->blink;
   return ptr;
   }

/*
 * flookup looks up id in flobal symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */
struct fentry *flookup(id)
char *id;
   {
   register struct fentry *ptr;

   ptr = fhash[FHasher(id)];
   while (ptr != NULL && ptr->name != id) {
      ptr = ptr->blink;
      }
   return ptr;
   }

/*
 * glookup looks up id in global symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */
struct gentry *glookup(id)
char *id;
   {
   register struct gentry *ptr;

   ptr = ghash[GHasher(id)];
   while (ptr != NULL && ptr->name != id) {
      ptr = ptr->blink;
      }
   return ptr;
   }

/*
 * clookup looks up id in constant symbol table and returns pointer to
 *  to it if found or NULL if not present.
 */
static struct centry *clookup(image,flag)
char *image;
int flag;
   {
   register struct centry *ptr;

   ptr = chash[CHasher(image)];
   while (ptr != NULL && (ptr->image != image || ptr->flag != flag))
      ptr = ptr->blink;

   return ptr;
   }

#ifdef DeBug
/*
 * symdump - dump symbol tables.
 */
void symdump()
   {
   struct pentry *proc;

   gdump();
   cdump();
   rdump();
   fdump();
   for (proc = proc_lst; proc != NULL; proc = proc->next) {
      fprintf(stderr,"\n");
      fprintf(stderr,"Procedure %s\n", proc->sym_entry->name);
      ldump(proc->lhash);
      }
   }

/*
 * prt_flgs - print flags from a symbol table entry.
 */
static void prt_flgs(flags)
int flags;
   {
   if (flags & F_Global)
      fprintf(stderr, " F_Global");
   if (flags & F_Proc)
      fprintf(stderr, " F_Proc");
   if (flags & F_Record)
      fprintf(stderr, " F_Record");
   if (flags & F_Dynamic)
      fprintf(stderr, " F_Dynamic");
   if (flags & F_Static)
      fprintf(stderr, " F_Static");
   if (flags & F_Builtin)
      fprintf(stderr, " F_Builtin");
   if (flags & F_StrInv)
      fprintf(stderr, " F_StrInv");
   if (flags & F_ImpError)
      fprintf(stderr, " F_ImpError");
   if (flags & F_Argument)
      fprintf(stderr, " F_Argument");
   if (flags & F_IntLit)
      fprintf(stderr, " F_IntLit");
   if (flags & F_RealLit)
      fprintf(stderr, " F_RealLit");
   if (flags & F_StrLit)
      fprintf(stderr, " F_StrLit");
   if (flags & F_CsetLit)
      fprintf(stderr, " F_CsetLit");
   if (flags & F_Field)
      fprintf(stderr, " F_Field");
   fprintf(stderr, "\n");
   }
/*
 * ldump displays local symbol table to stderr.
 */

void ldump(lhash)
struct lentry **lhash;
   {
   register int i;
   register struct lentry *lptr;

   fprintf(stderr,"   Dump of local symbol table\n");
   fprintf(stderr,"     address                 name  globol-ref  flags\n");
   for (i = 0; i < LHSize; i++)
      for (lptr = lhash[i]; lptr != NULL; lptr = lptr->blink) {
         fprintf(stderr,"    %8x %20s    ", lptr, lptr->name);
         if (lptr->flag & F_Global)
            fprintf(stderr, "%8x ", lptr->val.global);
         else
            fprintf(stderr, "       - ");
         prt_flgs(lptr->flag);
         }
   fflush(stderr);
   }

/*
 * gdump displays global symbol table to stderr.
 */

void gdump()
   {
   register int i;
   register struct gentry *gptr;

   fprintf(stderr,"\n");
   fprintf(stderr,"Dump of global symbol table\n");
   fprintf(stderr,"  address                 name  nargs  flags\n");
   for (i = 0; i < GHSize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink) {
         fprintf(stderr," %8x %20s   %4d ", gptr,
		gptr->name, gptr->nargs);
         prt_flgs(gptr->flag);
         }
   fflush(stderr);
   }

/*
 * cdump displays constant symbol table to stderr.
 */

void cdump()
   {
   register int i;
   register struct centry *cptr;

   fprintf(stderr,"\n");
   fprintf(stderr,"Dump of constant symbol table\n");
   fprintf(stderr,
      "  address  value                                      flags\n");
   for (i = 0; i < CHSize; i++)
      for (cptr = chash[i]; cptr != NULL; cptr = cptr->blink) {
         fprintf(stderr," %8x  %-40.40s  ", cptr, cptr->image);
         prt_flgs(cptr->flag);
         }
   fflush(stderr);
   }

/*
 * fdump displays field symbol table to stderr.
 */
void fdump()
   {
   int i;
   struct par_rec *prptr;
   struct fentry *fp;

   fprintf(stderr,"\n");
   fprintf(stderr,"Dump of field symbol table\n");
   fprintf(stderr,
      "  address                field  global-ref  offset\n");
   for (i = 0; i < FHSize; i++)
      for (fp = fhash[i]; fp != NULL; fp = fp->blink) {
         fprintf(stderr," %8x %20s\n", fp, fp->name);
         for (prptr = fp->rlist; prptr != NULL; prptr = prptr->next)
            fprintf(stderr,"                                  %8x    %4d\n",
               prptr->sym_entry, prptr->offset);
         }
   fflush(stderr);
   }

/*
 * prt_flds - print a list of fields stored in reverse order.
 */
static void prt_flds(f)
struct fldname *f;
   {
   if (f == NULL)
     return;
   prt_flds(f->next);
   fprintf(stderr, "  %s", f->name);
   }

/*
 * rdump displays list of records and their fields.
 */
void rdump()
   {
   struct rentry *rp;

   fprintf(stderr,"\n");
   fprintf(stderr,"Dump of record list\n");
   fprintf(stderr, " global-ref   fields\n");
   for (rp = rec_lst; rp != NULL; rp = rp->next) {
      fprintf(stderr, "   %8x ", rp->sym_entry);
      prt_flds(rp->fields);
      fprintf(stderr, "\n");
      }
   }
#endif					/* DeBug */

/*
 * alcloc allocates a local symbol table entry, fills in fields with
 *  specified values and returns pointer to new entry.  
 */
static struct lentry *alcloc(blink, name, flag)
struct lentry *blink;
char *name;
int flag;
   {
   register struct lentry *lp;

   lp = NewStruct(lentry);
   lp->blink = blink;
   lp->name = name;
   lp->flag = flag;
   return lp;
   }

/*
 * alcfld allocates a field symbol table entry, fills in the entry with
 *  specified values and returns pointer to new entry.  
 */
static struct fentry *alcfld(blink, name, rp)
struct fentry *blink;
char *name;
struct par_rec *rp;
   {
   register struct fentry *fp;

   fp = NewStruct(fentry);
   fp->blink = blink;
   fp->name = name;
   fp->rlist = rp;
   return fp;
   }

/*
 * alcglob allocates a global symbol table entry, fills in fields with
 *  specified values and returns pointer to new entry.  
 */
static struct gentry *alcglob(blink, name, flag)
struct gentry *blink;
char *name;
int flag;
   {
   register struct gentry *gp;

   gp = NewStruct(gentry);
   gp->blink = blink;
   gp->name = name;
   gp->flag = flag;
   return gp;
   }

/*
 * alclit allocates a constant symbol table entry, fills in fields with
 *  specified values and returns pointer to new entry.  
 */
static struct centry *alclit(blink, image, len, flag)
struct centry *blink;
char *image;
int len, flag;
   {
   register struct centry *cp;

   cp = NewStruct(centry);
   cp->blink = blink;
   cp->image = image;
   cp->length = len;
   cp->flag = flag;
   switch (flag) {
      case F_IntLit:
         cp->u.intgr = iconint(image);
         break;
      case F_CsetLit:
         cp->u.cset = bitvect(image, len);
         break;
      }
   return cp;
   }

/*
 * alcprec allocates an entry for the parent record list for a field.
 */
static struct par_rec *alcprec(rec, offset, next)
struct rentry *rec;
int offset;
struct par_rec *next;
   {
   register struct par_rec *rp;

   rp = NewStruct(par_rec);
   rp->rec= rec;
   rp->offset = offset;
   rp->next = next;
   return rp;
   }

/*
 * resolve - resolve the scope of undeclared identifiers.
 */
void resolve(proc)
struct pentry *proc;
   {
   struct lentry **lhash;
   register struct lentry *lp;
   struct gentry *gp;
   int i;
   char *id;

   lhash = proc->lhash;

   for (i = 0; i < LHSize; ++i) {
      lp = lhash[i];
      while (lp != NULL) {
         id = lp->name;
         if (lp->flag == 0) {				/* undeclared */
            if ((gp = try_gbl(id)) != NULL) {		/* check global */
               lp->flag = F_Global;
               lp->val.global = gp;
               }
            else {					/* implicit local */
               if (uwarn) {
                  fprintf(stderr, "%s undeclared identifier, procedure %s\n",
                     id, proc->name);
                  ++twarns;
                  }
               lp->flag = F_Dynamic;
               lp->next = proc->dynams;
               proc->dynams = lp;
               ++proc->ndynam;
               }
            }
         lp = lp->blink;
         }
      }
   }

/*
 * try_glb - see if the identifier is or should be a global variable.
 */
static struct gentry *try_gbl(id)
char *id;
   {
   struct gentry *gp;
   register struct implement *iptr;
   int nargs;
   int n;

   gp = glookup(id);
   if (gp == NULL) {
      /*
       * See if it is a built-in function.
       */
      iptr = db_ilkup(id, bhash);
      if (iptr == NULL)
         return NULL;
      else {
        if (iptr->in_line == NULL)
           nfatal(NULL, "built-in function not installed", id);
         nargs = iptr->nargs;
         if (nargs > 0 && iptr->arg_flgs[nargs - 1] & VarPrm)
            nargs = -nargs;
         gp = putglob(id, F_Global | F_Builtin);
         gp->val.builtin = iptr;

         n = n_arg_sym(iptr);
         if (n > max_sym)
            max_sym = n;
         }
      }
   return gp;
   }

/*
 * invoc_grp - called when "invocable all" is encountered.
 */
void invoc_grp(grp)
char *grp;
   {
   if (grp == spec_str("all"))
      str_inv = 1; /* enable full string invocation */
   else
      tfatal("invalid operand to invocable", grp);
   }

/*
 * invocbl - indicate that the operator is needed for for string invocation.
 */
void invocbl(op, arity)
nodeptr op;
int arity;
   {
   struct strinv *si;
   
   si = NewStruct(strinv);
   si->op = op;
   si->arity = arity;
   si->next = strinvlst;
   strinvlst = si;  
   }

/*
 * chkstrinv - check to see what is needed for string invocation.
 */
void chkstrinv()
   {
   struct strinv *si;
   struct gentry *gp;
   struct implement *ip;
   char *op_name;
   int arity;
   int i;

   /*
    * A table of procedure blocks for operators is set up for use by
    *  string invocation.
    */
   op_tbl_sz = 0;
   fprintf(codefile, "\nstatic B_IProc(2) init_op_tbl[OpTblSz]");

   if (str_inv) {
      /*
       * All operations must be available for string invocation. Make sure all
       *  built-in functions have either been hidden by global declarations
       *  or are in global variables, make sure no global variables are
       *  optimized away, and make sure all operations are in the table of
       *  operations.
       */
      for (i = 0; i < IHSize; ++i)  /* built-in function table */
         for (ip = bhash[i]; ip != NULL; ip = ip->blink)
            try_gbl(ip->name);
      for (i = 0; i < GHSize; i++)  /* global symbol table */
         for (gp = ghash[i]; gp != NULL; gp = gp->blink)
            gp->flag |= F_StrInv;
      for (i = 0; i < IHSize; ++i)  /* operator table */
         for (ip = ohash[i]; ip != NULL; ip = ip->blink)
            opstrinv(ip);
      }
   else {
      /*
       * selected operations must be available for string invocation.
       */
      for (si = strinvlst; si != NULL; si = si->next) {
         op_name = Str0(si->op);
         if (isalpha(*op_name) || (*op_name == '_')) {
             /*
              * This needs to be something in a global variable: function,
              *  procedure, or constructor.
              */
             gp = try_gbl(op_name);
             if (gp == NULL)
                nfatal(si->op, "not available for string invocation", op_name);
             else
                gp->flag |= F_StrInv;
             }
         else {
             /*
              * must be an operator.
              */
             arity = si->arity;
             i = IHasher(op_name);
             for (ip = ohash[i]; ip != NULL && ip->op != op_name;
                 ip = ip->blink)
                 ;
             if (arity < 0) {
                /*
                 * Operators of all arities with this symbol.
                 */
                while (ip != NULL && ip->op == op_name) {
                   opstrinv(ip);
                   ip = ip->blink;
                   }
                }
             else {
                /*
                 * Operator of a specific arity.
                 */
                while (ip != NULL && ip->nargs != arity)
                   ip = ip->blink;
                if (ip == NULL || ip->op != op_name)
                   nfatal(si->op, "not available for string invocation",
                      op_name);
                else
                   opstrinv(ip);
                }
             }
         }
      }

   /*
    * Add definitions to the header file indicating the size of the operator
    *  table and finish the declaration in the code file.
    */
   if (op_tbl_sz == 0) {
      fprintf(inclfile, "#define OpTblSz 1\n");
      fprintf(inclfile, "int op_tbl_sz = 0;\n");
      fprintf(codefile, ";\n");
      }
   else {
      fprintf(inclfile, "#define OpTblSz %d\n", op_tbl_sz);
      fprintf(inclfile, "int op_tbl_sz = OpTblSz;\n");
      fprintf(codefile, "\n   };\n");
      }
   }

/*
 * opstrinv - set up string invocation for an operator.
 */
static void opstrinv(ip)
struct implement *ip;
   {
   char c1, c2;
   char *name;
   char *op;
   register char *s;
   int nargs;
   int n;

   if (ip == NULL || ip->iconc_flgs & InStrTbl) 
      return;

   /*
    * Keep track of the maximum number of argument symbols in any operation
    *  so type inference can allocate enough storage for the worst case of
    *  general invocation.
    */
   n = n_arg_sym(ip);
   if (n > max_sym)
      max_sym = n;

   name = ip->name;
   c1 = ip->prefix[0];
   c2 = ip->prefix[1];
   op = ip->op;
   nargs = ip->nargs;
   if (ip->arg_flgs[nargs - 1] & VarPrm)
      nargs = -nargs;   /* indicate varargs with negative number of params */

   if (op_tbl_sz++ == 0) {
       fprintf(inclfile, "\n");
       fprintf(codefile, " = {\n");
       }
   else
       fprintf(codefile, ",\n");
   implproto(ip);   /* output prototype */

   /*
    * Output procedure block for this operator into table used by string
    *  invocation.
    */
   fprintf(codefile, "   {T_Proc, 11, O%c%c_%s, %d, -1, 0, 0, {{%d, \"", c1, c2,
      name, nargs, strlen(op));
   for (s = op; *s != '\0'; ++s) {
      if (*s == '\\')
         fprintf(codefile, "\\");
      fprintf(codefile, "%c", *s);
      }
   fprintf(codefile, "\"}}}");
   ip->iconc_flgs |= InStrTbl;
   }

/*
 * n_arg_sym - determine the number of argument symbols (dereferenced
 *  and undereferenced arguments are separate symbols) for an operation
 *  in the data base.
 */
int n_arg_sym(ip)
struct implement *ip;
   {
   int i;
   int num;

   num = 0;
   for (i = 0; i < ip->nargs; ++i) {
      if (ip->arg_flgs[i] & RtParm)
         ++num;
      if (ip->arg_flgs[i] & DrfPrm)
         ++num;
      }
   return num;
   }
