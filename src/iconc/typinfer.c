/*
 * typinfer.c - routines to perform type inference.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ctoken.h"
#include "cglobals.h"
#include "ccode.h"
#include "cproto.h"
#ifdef TypTrc
#ifdef HighResTime
#include <sys/time.h>
#include <sys/resource.h>
#endif					/* HighResTime */
#endif					/* TypTrc */

/*
 * Information about co-expressions is keep on a list.
 */
struct t_coexpr {
   nodeptr n;               /* code for co-expression */
   int typ_indx;            /* relative type number (index) */
   struct store *in_store;  /* store entry into co-expression via activation */
   struct store *out_store; /* store at end of co-expression */
#ifdef OptimizeType
   struct typinfo *act_typ;   /* types passed via co-expression activation */
   struct typinfo *rslt_typ;  /* types resulting from "co-expression return" */
#else					/* OptimizeType */
   unsigned int *act_typ;   /* types passed via co-expression activation */
   unsigned int *rslt_typ;  /* types resulting from "co-expression return" */
#endif					/* OptimizeType */
   int iteration;
   struct t_coexpr *next;
   };

struct t_coexpr *coexp_lst;

#ifdef TypTrc
extern int typealloc;		/* flag to account for allocation */
extern long typespace;		/* amount of space for type inference */
#endif					/* TypTrc */

struct symtyps *cur_symtyps; /* maps run-time routine symbols to types */

/*
 * argtyps is the an array of types large enough to accommodate the argument
 *  list of any operation.
 */
struct argtyps {
   struct argtyps *next;
#ifdef OptimizeType
   struct typinfo *types[1];  /* actual size is max_prm */
#else					/* OptimizeType */
   unsigned int *types[1];  /* actual size is max_prm */
#endif					/* OptimizeType */
   };

/*
 * prototypes for static functions.
 */
#ifdef OptimizeType
void         and_bits_to_packed (struct typinfo *src, 
                                     struct typinfo *dest, int size);
struct typinfo *alloc_typ   (int n_types);
unsigned int   *alloc_mem_typ   (unsigned int n_types);
int             bitset      (struct typinfo *typ, int bit);
void         clr_typ     (struct typinfo *type, unsigned int bit);
void         clr_packed   (struct typinfo *src, int nsize);
void         cpy_packed_to_packed (struct typinfo *src,
				     struct typinfo *dest, int nsize);
static void         deref_lcl   (struct typinfo *src,
                                     struct typinfo *dest);
static int             findloops   ( struct node *n, int resume,
                                     struct typinfo *rslt_type);
static void         gen_inv     (struct typinfo *prc_typ, nodeptr n);
int             has_type    (struct typinfo *typ, int typcd, int clear);
static void         infer_impl  (struct implement *impl,
                                     nodeptr n, struct symtyps *symtyps,
                                     struct typinfo *rslt_typ);
int             is_empty    (struct typinfo *typ);
int             mrg_packed_to_packed (struct typinfo *src,
				     struct typinfo *dest, int nsize);
int             other_type  (struct typinfo *typ, int typcd);
static void         set_ret     (struct typinfo *typ);
void         set_typ     (struct typinfo *type, unsigned int bit);
void         typcd_bits  (int typcd, struct type *typ);
static void         typ_deref   (struct typinfo *src,
                                     struct typinfo *dest, int chk);
int             xfer_packed_to_bits (struct typinfo *src,
                                     struct typinfo *dest, int nsize);
#else					/* OptimizeType */
unsigned int   *alloc_typ   (int n_types);
int             bitset      (unsigned int *typ, int bit);
void         clr_typ     (unsigned int *type, unsigned int bit);
static void         deref_lcl   (unsigned int *src, unsigned int *dest);
static int             findloops   ( struct node *n, int resume,
                                     unsigned int *rslt_type);
static void         gen_inv     (unsigned int *prc_typ, nodeptr n);
int             has_type    (unsigned int *typ, int typcd, int clear);
static void         infer_impl  (struct implement *impl,
                                     nodeptr n, struct symtyps *symtyps,
                                     unsigned int *rslt_typ);
int             is_empty    (unsigned int *typ);
int             other_type  (unsigned int *typ, int typcd);
static void         set_ret     (unsigned int *typ);
void         set_typ     (unsigned int *type, unsigned int bit);
void         typcd_bits  (int typcd, struct type *typ);
static void         typ_deref (unsigned int *src, unsigned int *dest, int chk);
#endif					/* OptimizeType */

static void         abstr_new   (struct node *n, struct il_code *il);
static void         abstr_typ   (struct il_code *il, struct type *typ);
static struct store   *alloc_stor  (int stor_sz, int n_types);
static void         chk_succ    (int ret_flag, struct store *susp_stor);
static struct store   *cpy_store   (struct store *source);
static int             eval_cond   (struct il_code *il);
static void         free_argtyp (struct argtyps *argtyps);
static void         free_store  (struct store *store);
static void         free_wktyp  (struct type *typ);
static void         find_new    (struct node *n);
static struct argtyps *get_argtyp  (void);
static struct store   *get_store   (int clear);
static struct type    *get_wktyp   (void);
static void         infer_act   (nodeptr n);
static void         infer_con   (struct rentry *rec, nodeptr n);
static int             infer_il    (struct il_code *il);
static void         infer_nd    (nodeptr n);
static void         infer_prc   (struct pentry *proc, nodeptr n);
static void         mrg_act     (struct t_coexpr *coexp,
                                     struct store *e_store,
                                     struct type *rslt_typ);
static void         mrg_store   (struct store *source, struct store *dest);
static void         side_effect (struct il_code *il);
static struct symtyps *symtyps     (int nsyms);

#ifdef TypTrc
static void         prt_d_typ   (FILE *file, struct typinfo *typ);
static void         prt_typ     (FILE *file, struct typinfo *typ);
#endif					/* TypTrc */

#define CanFail   1

/*
 * cur_coexp is non-null while performing type inference on code from a
 *  create expression. If it is null, the possible current co-expressions
 *  must be found from cur_proc.
 */
struct t_coexpr *cur_coexp = NULL;

struct gentry **proc_map;    /* map procedure types to symbol table entries */
struct rentry **rec_map;     /* map record types to record information */
struct t_coexpr **coexp_map; /* map co-expression types to information */

struct typ_info *type_array;

static int num_new;   /* number of types supporting "new" abstract type comp */

/*
 * Data base component codes are mapped to type inferencing information 
 *  using an array.
 */
struct compnt_info {
   int frst_bit;        /* first bit in bit vector allocated to component */
   int num_bits;        /* number of bits allocated to this component */
   struct store *store; /* maps component "reference" to the type it holds */ 
   };
static struct compnt_info *compnt_array;

static unsigned int frst_fld;   /* bit number of 1st record field */
static unsigned int n_fld;      /* number of record fields */
static unsigned int frst_gbl;   /* bit number of 1st global reference type */
static unsigned int n_gbl;      /* number of global variables */
static unsigned int n_nmgbl;    /* number of named global variables */
static unsigned int frst_loc;   /* bit number of 1st local reference type */
static unsigned int n_loc;      /* maximum number of locals in any procedure */

static unsigned int nxt_bit;    /* next unassigned bit in bit vector */
unsigned int n_icntyp;   /* number of non-variable types */
unsigned int n_intrtyp;  /* number of types in intermediate values */
static unsigned int n_rttyp;    /* number of types in runtime computations */
unsigned int val_mask;   /* mask for non-var types in last int of type */

unsigned int null_bit;   /* bit for null type */
unsigned int str_bit;    /* bit for string type */
unsigned int cset_bit;   /* bit for cset type */
unsigned int int_bit;    /* bit for integer type */
unsigned int real_bit;   /* bit for real type */

static struct store *fld_stor;   /* record fields */

static int *cur_new;      /* allocated types for current operation */

static struct store *succ_store = NULL; /* current success store */
static struct store *fail_store = NULL; /* current failure store */

static struct store *dummy_stor;
static struct store *store_pool = NULL; /* free list of store structs */

static struct type *type_pool = NULL;          /* free list of type structs */
static struct type cur_rslt = {0, NULL, NULL}; /* result type of operation */

static struct argtyps *argtyp_pool = NULL; /* free list of arg type arrays */
static struct argtyps *arg_typs = NULL;    /* current arg type array */

static int num_args; /* number of arguments for current operation */
static int n_vararg; /* size of variable part of arg list to run-time routine */

#ifdef OptimizeType
static struct typinfo *any_typ; /* type bit vector with all bits on */
struct typinfo *free_typinfo = NULL;
struct typinfo *start_typinfo = NULL;
struct typinfo *high_typinfo = NULL;
struct typinfo *low_typinfo = NULL;
#else					/* OptimizeType */
static unsigned int *any_typ; /* type bit vector with all bits on */
#endif					/* OptimizeType */

long changed;  /* number of changes to type information in this iteration */
int iteration; /* iteration number for type inferencing */

#ifdef TypTrc
static FILE *trcfile = NULL;	/* output file pointer for tracing */
static char *trcname = NULL;	/* output file name for tracing */
static char *trc_indent = "";
#endif					/* TypTrc */


/*
 * typeinfer - infer types of operands. If "do_typinfer" is set, actually
 *   do abstract interpretation, otherwise assume any type for all operands.
 */
void typeinfer()
   {
   struct gentry *gptr;
   struct lentry *lptr;
   nodeptr call_main;
   struct pentry *p;
   struct rentry *rec;
   struct t_coexpr *coexp;
   struct store *init_store;
   struct store *f_store;
#ifdef OptimizeType
   struct typinfo *type;
#else					/* OptimizeType */
   unsigned int *type;
#endif					/* OptimizeType */
   struct implement *ip;
   struct lentry **lhash;
   struct lentry **vartypmap;
   int i, j, k;
   int size;
   int flag;

#ifdef TypTrc
   /*
    * Set up for type tracing.
    */
   long start_infer, end_infer;

#ifdef HighResTime
   struct rusage rusage;

   getrusage(RUSAGE_SELF, &rusage);
   start_infer = rusage.ru_utime.tv_sec*1000 + rusage.ru_utime.tv_usec/1000;
#else					/* HighResTime */
   start_infer = millisec();
#endif					/* HighResTime */

   typealloc = 1;		/* note allocation in this phase */

   trcname = getenv("TYPTRC");

   if (trcname != NULL && strlen(trcname) != 0) {

      if (trcname[0] == '|') {
         FILE *popen();

         trcfile = popen(trcname+1, "w");
         }
      else

      trcfile = fopen(trcname, "w");

      if (trcfile == NULL) {
         fprintf(stderr, "TYPTRC: cannot open %s\n", trcname);
         fflush(stderr);
         exit(EXIT_FAILURE);
         }
      }
#endif					/* TypTrc */

   /*
    * Make sure max_prm is large enough for any run-time routine.
    */
   for (i = 0; i < IHSize; ++i)
      for (ip = bhash[i]; ip != NULL; ip = ip->blink)
         if (ip->nargs > max_prm)
           max_prm = ip->nargs;
   for (i = 0; i < IHSize; ++i)
      for (ip = ohash[i]; ip != NULL; ip = ip->blink)
         if (ip->nargs > max_prm)
           max_prm = ip->nargs;

   /*
    * Allocate an arrays to map data base type codes and component codes 
    *  to type inferencing information.
    */
   type_array = (struct typ_info *)alloc((unsigned int)(num_typs *
      sizeof(struct typ_info)));
   compnt_array = (struct compnt_info *)alloc((unsigned int)(num_cmpnts *
      sizeof(struct compnt_info)));

   /*
    * Find those types that support the "new" abstract type computation
    *  assign to them locations in the arrays of allocated types associated
    *  with operation invocations. Also initialize the number of type bits.
    *  Types with no subtypes have one bit. Types allocated with the the "new"
    *  abstract have a default sub-type that is allocated here. Procedures
    *  have a subtype to for string invocable operators. Co-expressions
    *  have a subtype for &main. Records are handled below.
    */
   num_new = 0;
   for (i = 0; i < num_typs; ++i) {
      if (icontypes[i].support_new)
         type_array[i].new_indx = num_new++;
      type_array[i].num_bits = 1;   /* reserve one type bit */
      }
   type_array[list_typ].num_bits = 2;  /* default & list for arg to main() */

   cur_coexp = NewStruct(t_coexpr);
   cur_coexp->n = NULL;
   cur_coexp->next = NULL;
   coexp_lst = cur_coexp;

   if (do_typinfer) {
      /*
       * Go through the  syntax tree for each procedure locating program
       *  points that may create structures at run time. Allocate the
       *  appropriate structure type(s) to each such point.
       */
      for (p = proc_lst; p != NULL; p = p->next) {
         if (p->nargs < 0)
            p->arg_lst = type_array[list_typ].num_bits++; /* list for varargs */
         find_new(Tree1(p->tree));  /* initial clause */
         find_new(Tree2(p->tree));  /* body of procedure */
         }
      }

   /*
    * Allocate a type number for each record type (use record number for
    *  offset) and a variable type number for each field.
    */
   n_fld = 0;
   if (rec_lst == NULL) {
      type_array[rec_typ].num_bits = 0;
      rec_map = NULL;
      }
   else {
      type_array[rec_typ].num_bits = rec_lst->rec_num + 1;
      rec_map = (struct rentry **)alloc(
         (unsigned int)((rec_lst->rec_num + 1)*sizeof(struct rentry *)));
      for (rec = rec_lst; rec != NULL; rec = rec->next) {
         rec->frst_fld = n_fld;
         n_fld += rec->nfields;
         rec_map[rec->rec_num] = rec;
         }
      }

   /*
    * Allocate type numbers to global variables. Don't count those procedure
    *  variables that are no longer referenced in the syntax tree. Do count
    *  static variables. Also allocate types to procedures, built-in functions,
    *  record constructors.
    */
   n_gbl = 0; 
   for (i = 0; i < GHSize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink) {
         flag = gptr->flag;
         if (flag & F_SmplInv)
            gptr->index = -1;   /* unused: set to something not a valid type */
         else {
            gptr->index = n_gbl++;
            if (flag & (F_Proc | F_Record | F_Builtin))
               gptr->init_type = type_array[proc_typ].num_bits++;
            }
         if (flag & F_Proc) {
            for (lptr = gptr->val.proc->statics; lptr != NULL;lptr = lptr->next)
               lptr->val.index = n_gbl++;
            }
         }
   n_nmgbl = n_gbl;

   /*
    * Determine relative bit numbers for predefined variable types that
    *  are treated as sets of global variables.
    */
   for (i = 0; i < num_typs; ++i)
      if (icontypes[i].deref == DrfGlbl)
         type_array[i].frst_bit = n_gbl++; /* converted to absolute later */

   proc_map = (struct gentry **)alloc(
      (unsigned int)((type_array[proc_typ].num_bits)*sizeof(struct gentry *)));
   proc_map[0] = NULL; /* proc type for string invocable operators */
   for (i = 0; i < GHSize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink) {
         flag = gptr->flag;
         if (!(flag & F_SmplInv) && (flag & (F_Proc | F_Record | F_Builtin)))
            proc_map[gptr->init_type] = gptr;
         }

   /*
    * Allocate type numbers to local variables. The same numbers are reused
    *  in different procedures.
    */
   n_loc = 0;
   for (p = proc_lst; p != NULL; p = p->next) {
      i = Abs(p->nargs);
      for (lptr = p->args; lptr != NULL; lptr = lptr->next)
         lptr->val.index = --i;
      i = Abs(p->nargs);
      for (lptr = p->dynams; lptr != NULL; lptr = lptr->next)
         lptr->val.index = i++;
      n_loc = Max(n_loc, i);

      /*
       * produce a mapping from the variable types used in this procedure
       *  to the corresponding symbol table entries.
       */
      if (n_gbl + n_loc == 0)
         vartypmap = NULL;
      else
         vartypmap = (struct lentry **)alloc(
            (unsigned int)((n_gbl + n_loc)*sizeof(struct lentry *)));
      for (i = 0; i < n_gbl + n_loc; ++i)
          vartypmap[i] = NULL; /* no entries for foreign statics */
      p->vartypmap = vartypmap;
      lhash = p->lhash;
      for (i = 0; i < LHSize; ++i) {
         for (lptr = lhash[i]; lptr != NULL; lptr = lptr->blink) {
            switch (lptr->flag) {
               case F_Global:
                  gptr = lptr->val.global;
                  if (!(gptr->flag & F_SmplInv))
                     vartypmap[gptr->index] = lptr;
                  break;
               case F_Static:
                  vartypmap[lptr->val.index] = lptr;
                  break;
               case F_Dynamic:
               case F_Argument:
                  vartypmap[n_gbl + lptr->val.index] = lptr;
                  }
            }
         }
      }

   /*
    * There is a component reference subtype for every subtype of the
    *  associated aggregate type.
    */
   for (i = 0; i < num_cmpnts; ++i)
      compnt_array[i].num_bits = type_array[typecompnt[i].aggregate].num_bits;

   /*
    * Assign bits for non-variable (first-class) types.
    */
   nxt_bit = 0;
   for (i = 0; i < num_typs; ++i)
      if (icontypes[i].deref == DrfNone) {
         type_array[i].frst_bit = nxt_bit;
         nxt_bit += type_array[i].num_bits;
         }

   n_icntyp = nxt_bit; /* number of first-class types */

   /*
    * Load some commonly needed bit numbers into global variable.
    */
   null_bit = type_array[null_typ].frst_bit;
   str_bit = type_array[str_typ].frst_bit;
   cset_bit = type_array[cset_typ].frst_bit;
   int_bit = type_array[int_typ].frst_bit;
   real_bit = type_array[real_typ].frst_bit;

   /*
    * Assign bits for predefined variable types that are not treated as
    *   sets of globals.
    */
   for (i = 0; i < num_typs; ++i)
      if (icontypes[i].deref == DrfCnst || icontypes[i].deref == DrfSpcl) {
         type_array[i].frst_bit = nxt_bit;
         nxt_bit += type_array[i].num_bits;
         }

   /*
    * Assign bits to aggregate compontents that are variables.
    */
   for (i = 0; i < num_cmpnts; ++i)
      if (typecompnt[i].var) {
         compnt_array[i].frst_bit = nxt_bit;
         nxt_bit += compnt_array[i].num_bits;
         }

   /*
    * Assign bits to record fields and named variables.
    */
   frst_fld = nxt_bit;
   nxt_bit += n_fld;
   frst_gbl = nxt_bit;
   nxt_bit += n_gbl;
   frst_loc = nxt_bit;
   nxt_bit += n_loc;

   /*
    * Convert from relative to ablsolute bit numbers for predefined variable
    *  types that are treated as sets of global variables.
    */
   for (i = 0; i < num_typs; ++i)
      if (icontypes[i].deref == DrfGlbl)
         type_array[i].frst_bit += frst_gbl;

   n_intrtyp = nxt_bit; /* number of types for intermediate values */

   /*
    * Assign bits to aggregate compontents that are not variables. These
    *  are the runtime system's internal descriptor reference types.
    */
   for (i = 0; i < num_cmpnts; ++i)
      if (!typecompnt[i].var) {
         compnt_array[i].frst_bit = nxt_bit;
         nxt_bit += compnt_array[i].num_bits;
         }

   n_rttyp = nxt_bit; /* total size of type system */

#ifdef TypTrc
   if (trcfile != NULL) {
      /*
       * Output a summary of the type system.
       */
      for (i = 0; i < num_typs; ++i) {
         fprintf(trcfile, "%s", icontypes[i].id);
         if (strcmp(icontypes[i].id, icontypes[i].abrv) != 0)
            fprintf(trcfile, "(%s)", icontypes[i].abrv);
         fprintf(trcfile, " sub-types: %d\n", type_array[i].num_bits);
         }
      }
#endif					/* TypTrc */

   /*
    * The division between bits for first-class types and variables types
    *  generally occurs in the middle of a word. Set up a mask for extracting
    *  the first-class types from this word.
    */
   val_mask = 0;
   i = n_icntyp - (NumInts(n_icntyp) - 1) * IntBits;
   while (i--)
      val_mask = (val_mask << 1) | 1;

   if (do_typinfer) {
      /*
       * Create stores large enough for the component references. These
       *  are global to the entire program, rather than being propagated
       *  from node to node in the syntax tree.
       */
      for (i = 0; i < num_cmpnts; ++i) {
         if (i == str_var)
            size = n_intrtyp;
         else
            size = n_icntyp;
         compnt_array[i].store = alloc_stor(compnt_array[i].num_bits, size);
         }
      fld_stor = alloc_stor(n_fld, n_icntyp);

      dummy_stor = get_store(0);

      /*
       * First list is arg to main: a list of strings.
       */
      set_typ(compnt_array[lst_elem].store->types[1], str_typ);
      }

   /*
    * Set up a type bit vector with all bits on.
    */
#ifdef OptimizeType
   any_typ = alloc_typ(n_rttyp);
   any_typ->bits = alloc_mem_typ(DecodeSize(any_typ->packed));
   for (i = 0; i < NumInts(n_rttyp); ++i)
      any_typ->bits[i] = ~(unsigned int)0;
#else					/* OptimizeType */
   any_typ = alloc_typ(n_rttyp);
   for (i = 0; i < NumInts(n_rttyp); ++i)
      any_typ[i] = ~(unsigned int)0;
#endif					/* OptimizeType */

   /*
    *  Initialize stores and return values for procedures. Also initialize
    *   flag indicating whether the procedure can be executed.
    */
   call_main = NULL;
   for (p = proc_lst; p != NULL; p = p->next) {
      if (do_typinfer) {
         p->iteration = 0;
         p->ret_typ = alloc_typ(n_intrtyp);
         p->coexprs = alloc_typ(n_icntyp);
         p->in_store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (p->ret_flag & DoesSusp)
            p->susp_store = alloc_stor(n_gbl, n_icntyp);
         else
            p->susp_store = NULL;
         for (i = Abs(p->nargs); i < n_loc; ++i)
            set_typ(p->in_store->types[n_gbl + i], null_bit);
         if (p->nargs < 0)
            set_typ(p->in_store->types[n_gbl + Abs(p->nargs) - 1],
               type_array[list_typ].frst_bit + p->arg_lst);
         if (strcmp(p->name, "main") == 0) {
            /*
             * create a the initial call to main with one list argument.
             */
            call_main = invk_main(p);
            call_main->type = alloc_typ(n_intrtyp);
            Tree2(call_main)->type = alloc_typ(n_intrtyp);
            set_typ(Tree2(call_main)->type, type_array[list_typ].frst_bit + 1);
            call_main->store = alloc_stor(n_gbl + n_loc, n_icntyp);
            }
         p->out_store = alloc_stor(n_gbl, n_icntyp);
         p->reachable = 0;
         }
      else
         p->reachable = 1;
      /*
       * Analyze the code of the procedure to determine where to place stores
       *  that survive iterations of type inferencing. Note, both the initial
       *  clause and the body of the procedure are bounded.
       */
      findloops(Tree1(p->tree), 0, NULL);
      findloops(Tree2(p->tree), 0, NULL);
      }

   /*
    * If type inferencing is suppressed, we have set up very conservative
    *   type information and will do no inferencing.
    */
   if (!do_typinfer)
      return;

   if (call_main == NULL)
      return;         /* no main procedure, cannot continue */
   if (tfatals > 0)
      return;         /* don't do inference if there are fatal errors */

   /*
    * Construct mapping from co-expression types to information
    *  about the co-expressions and finish initializing the information.
    */
   i = type_array[coexp_typ].num_bits;
   coexp_map = (struct t_coexpr **)alloc(
      (unsigned int)(i * sizeof(struct t_coexpr *)));
   for (coexp = coexp_lst; coexp != NULL; coexp = coexp->next) {
       coexp_map[--i] = coexp;
       coexp->typ_indx = i;
       coexp->in_store = alloc_stor(n_gbl + n_loc, n_icntyp);
       coexp->out_store = alloc_stor(n_gbl + n_loc, n_icntyp);
       coexp->act_typ = alloc_typ(n_intrtyp);
       coexp->rslt_typ = alloc_typ(n_intrtyp);
       coexp->iteration = 0;
       }

   /*
    * initialize globals
    */
   init_store = get_store(1);
   for (i = 0; i < GHSize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink) {
         flag = gptr->flag;
         if (!(flag & F_SmplInv)) {
            type = init_store->types[gptr->index];
            if (flag & (F_Proc | F_Record | F_Builtin))
               set_typ(type, type_array[proc_typ].frst_bit + gptr->init_type);
            else
               set_typ(type, null_bit);
            }
         }

   /*
    * Initialize types for predefined variable types.
    */
   for (i = 0; i < num_typs; ++i) {
      type = NULL;
      switch (icontypes[i].deref) {
         case DrfGlbl:
            /*
             * Treated as a global variable.
             */
            type = init_store->types[type_array[i].frst_bit - frst_gbl];
            break;
         case DrfCnst:
            /*
             * Type doesn't change so keep one copy.
             */
            type = alloc_typ(n_intrtyp);
            type_array[i].typ = type;
            break;
         }
      if (type != NULL) {
          /*
           * Determine which types are in the initial type for this variable.
           */
          for (j = 0; j < num_typs; ++j) {
             if (icontypes[i].typ[j] != '.') {
                for (k = 0; k < type_array[j].num_bits; ++k)
                   set_typ(type, type_array[j].frst_bit + k);
                }
             }
          }
      }

   f_store = get_store(1);

   /*
    * Type inferencing iterates over the program until a fixed point is
    *  reached.
    */
   changed = 1L;    /* force first iteration */
   iteration = 0;
   if (verbose > 1)
      fprintf(stderr, "type inferencing: ");

   while (changed > 0L) {
     changed = 0L;
     ++iteration;

#ifdef TypTrc
     if (trcfile != NULL)
        fprintf(trcfile, "**** iteration %d ****\n", iteration);
#endif					/* TypTrc */

     /*
      * Start at the implicit initial call to the main procedure. Inferencing
      *  walks the call graph from here.
      */
     succ_store = cpy_store(init_store);
     fail_store = f_store;
     infer_nd(call_main);

     /*
      * If requested, monitor the progress of inferencing.
      */
     switch (verbose) {
        case 0:
        case 1:
           break;
        case 2:
           fprintf(stderr, ".");
           break;
        default: /* > 2 */
           if (iteration != 1)
              fprintf(stderr, ", ");
           fprintf(stderr, "%ld", changed);
        }
     }

   /*
    * Type inferencing is finished, complete any diagnostic output.
    */
   if (verbose > 1)
      fprintf(stderr, "\n");

#ifdef TypTrc
     if (trcfile != NULL) {

#ifdef HighResTime
        getrusage(RUSAGE_SELF, &rusage);
        end_infer = rusage.ru_utime.tv_sec*1000 + rusage.ru_utime.tv_usec/1000;
#else					/* HighResTime */
        end_infer = millisec();
#endif					/* HighResTime */
        fprintf(trcfile, "\n**** inferencing time: %ld milliseconds\n", 
           end_infer - start_infer);
        fprintf(trcfile, "\n**** inferencing space: %ld bytes\n",typespace);
        fclose(trcfile);
        }
   typealloc = 0;
#endif					/* TypTrc */
   }

/*
 * find_new - walk the syntax tree allocating structure types where
 *  operations create new structures.
 */
static void find_new(n)
struct node *n;
   {
   struct t_coexpr *coexp;
   struct node *cases;
   struct node *clause;
   int nargs;
   int i;

   n->new_types = NULL;
   switch (n->n_type) {
      case N_Cset:
      case N_Empty:
      case N_Id:
      case N_Int:
      case N_Next:
      case N_Real:
      case N_Str:
         break;

      case N_Bar:
      case N_Break:
      case N_Field:
      case N_Not:
         find_new(Tree0(n));
         break;

      case N_Alt:
      case N_Apply:
      case N_Limit:
      case N_Slist:
         find_new(Tree0(n));
         find_new(Tree1(n));
         break;

      case N_Activat:
         find_new(Tree1(n));
         find_new(Tree2(n));
         break;

      case N_If:
         find_new(Tree0(n));  /* control clause */
         find_new(Tree1(n));  /* then clause */
         find_new(Tree2(n));  /* else clause, may be N_Empty */
         break;

      case N_Create:
         /*
          * Allocate a sub-type for the co-expressions created here.
          */
         n->new_types = (int *)alloc((unsigned int)(sizeof(int)));
         n->new_types[0] = type_array[coexp_typ].num_bits++;
         coexp = NewStruct(t_coexpr);
         coexp->n = Tree0(n);
         coexp->next = coexp_lst;
         coexp_lst = coexp;
         find_new(Tree0(n));
         break;

      case N_Augop:
         abstr_new(n, Impl0(n)->in_line);  /* assignment */
         abstr_new(n, Impl1(n)->in_line);  /* the operation */
         find_new(Tree2(n));              /* 1st operand */
         find_new(Tree3(n));              /* 2nd operand */
         break;

      case N_Case:
         find_new(Tree0(n));  /* control clause */
         cases = Tree1(n);
         while (cases != NULL) {
            if (cases->n_type == N_Ccls) {
               clause = cases;
               cases = NULL;
               }
            else {
               clause = Tree1(cases);
               cases = Tree0(cases);
               }

            find_new(Tree0(clause));   /* value of clause */
            find_new(Tree1(clause));   /* body of clause */
            }
         if (Tree2(n) != NULL)
            find_new(Tree2(n));  /* deflt */
         break;

      case N_Invok:
         nargs = Val0(n);                  /* number of arguments */
         find_new(Tree1(n));               /* thing being invoked */
         for (i = 1; i <= nargs; ++i)
            find_new(n->n_field[i+1].n_ptr); /* arg i */
         break;

      case N_InvOp:
         /*
          * This is a call to an operation, this is what we must
          *  check for "new" abstract type computation.
          */
         nargs = Val0(n);                    /* number of arguments */
         abstr_new(n, Impl1(n)->in_line);     /* operation */
         for (i = 1; i <= nargs; ++i)
            find_new(n->n_field[i+1].n_ptr); /* arg i */
         break;

      case N_InvProc:
      case N_InvRec:
         nargs = Val0(n);                    /* number of arguments */
         for (i = 1; i <= nargs; ++i)
            find_new(n->n_field[i+1].n_ptr); /* arg i */
         break;

      case N_Loop:
         switch ((int)Val0(Tree0(n))) {
            case EVERY:
            case SUSPEND:
            case WHILE:
            case UNTIL:
               find_new(Tree1(n));   /* control clause */
               find_new(Tree2(n));   /* do clause - may be N_Empty*/
               break;

            case REPEAT:
               find_new(Tree1(n));   /* clause */
               break;
            }

      case N_Ret:
         if (Val0(Tree0(n)) == RETURN)
            find_new(Tree1(n));    /* value - may be N_Empty */
         break;

      case N_Scan:
         if (optab[Val0(Tree0(n))].tok.t_type == AUGQMARK)
            abstr_new(n, optab[asgn_loc].binary->in_line);
         find_new(Tree1(n));   /* subject */ 
         find_new(Tree2(n));   /* body */
         break;

      case N_Sect:
         abstr_new(n, Impl0(n)->in_line);     /* sectioning */
         if (Impl1(n) != NULL)
            abstr_new(n, Impl1(n)->in_line);  /* plus, minus, or nothing */
         find_new(Tree2(n));                 /* 1st operand */
         find_new(Tree3(n));                 /* 2nd operand */
         find_new(Tree4(n));                 /* 3rd operand */
         break;

      case N_SmplAsgn:
      case N_SmplAug:
         find_new(Tree3(n));
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }
   }

/*
 * abstr_new - find the abstract clauses in the implementation of an operation.
 *  If they indicate that the operations creates structures, allocate a
 *  type for the structures and associate it with the node in the syntax tree.
 */
static void abstr_new(n, il)
struct node *n;
struct il_code *il;
   {
   int i;
   int num_cases, indx;
   struct typ_info *t_info;

   if (il == NULL)
      return;

   switch (il->il_type) {
      case IL_New:
         /*
          * We have found a "new" construct in an abstract type computation.
          *  Make sure an array has been created to hold the types allocated
          *  to this call, then allocate the indicated type if one has not
          *  already been allocated.
          */
         if (n->new_types == NULL) {
            n->new_types = (int *)alloc((unsigned int)(num_new * sizeof(int)));
            for (i = 0; i < num_new; ++i)
               n->new_types[i] = -1;
            }
         t_info = &type_array[il->u[0].n];     /* index by type code */
         if (n->new_types[t_info->new_indx] < 0) {
             n->new_types[t_info->new_indx] = t_info->num_bits++;
#ifdef TypTrc
             if (trcfile != NULL)
                fprintf(trcfile, "%s (%d,%d) %s\n", n->n_file, n->n_line,
                   n->n_col, icontypes[il->u[0].n].id);
#endif					/* TypTrc */
             }
         i = il->u[1].n;            /* num args */
         indx = 2;
         while (i--)
            abstr_new(n, il->u[indx++].fld);
         break;

      case IL_If1:
         abstr_new(n, il->u[1].fld);
         break;

      case IL_If2:
         abstr_new(n, il->u[1].fld);
         abstr_new(n, il->u[2].fld);
         break;

      case IL_Tcase1:
         num_cases = il->u[1].n;
         indx = 2;
         for (i = 0; i < num_cases; ++i) {
            indx += 2;                        /* skip type info */
            abstr_new(n, il->u[indx++].fld);  /* action */
            }
         break;

      case IL_Tcase2:
         num_cases = il->u[1].n;
         indx = 2;
         for (i = 0; i < num_cases; ++i) {
            indx += 2;                        /* skip type info */
            abstr_new(n, il->u[indx++].fld);  /* action */
            }
         abstr_new(n, il->u[indx].fld);       /* default */
         break;

      case IL_Lcase:
         num_cases = il->u[0].n;
         indx = 1;
         for (i = 0; i < num_cases; ++i) {
            ++indx;		              /* skip selection num */
            abstr_new(n, il->u[indx++].fld);  /* action */
            }
         abstr_new(n, il->u[indx].fld);       /* default */
         break;

      case IL_Acase:
         abstr_new(n, il->u[2].fld);          /* C_integer action */
         if (largeints) 
            abstr_new(n, il->u[3].fld);       /* integer action */
         abstr_new(n, il->u[4].fld);          /* C_double action */
         break;

      case IL_Abstr:
      case IL_Inter:
      case IL_Lst:
      case IL_TpAsgn:
      case IL_Union:
         abstr_new(n, il->u[0].fld);
         abstr_new(n, il->u[1].fld);
         break;

      case IL_Compnt:
      case IL_Store:
      case IL_VarTyp:
         abstr_new(n, il->u[0].fld);
         break;

      case IL_Block:
      case IL_Call:
      case IL_Const:  /* should have been replaced by literal node */
      case IL_Err1:
      case IL_Err2:
      case IL_IcnTyp:
      case IL_Subscr:
      case IL_Var:
         break;

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(EXIT_FAILURE);
      }
   }

/*
 * alloc_stor - allocate a store with empty types.
 */
static struct store *alloc_stor(stor_sz, n_types)
int stor_sz;
int n_types;
   {
   struct store *stor;
   int i;

   /*
    * If type inferencing is disabled, we don't actually make use of
    *  any stores, but the initialization code asks for them anyway.
    */
   if (!do_typinfer)
      return NULL;

#ifdef OptimizeType
   stor = (struct store *)alloc((unsigned int)(sizeof(struct store) +
      ((stor_sz - 1) * sizeof(struct typinfo *))));
   stor->next = NULL;
   stor->perm = 1;
   for (i = 0; i < stor_sz; ++i) {
      stor->types[i] = (struct typinfo *)alloc_typ(n_types);
      }
#else					/* OptimizeType */
   stor = (struct store *)alloc((unsigned int)(sizeof(struct store) +
      ((stor_sz - 1) * sizeof(unsigned int *))));
   stor->next = NULL;
   stor->perm = 1;
   for (i = 0; i < stor_sz; ++i) {
      stor->types[i] = (unsigned int *)alloc_typ(n_types);
      }
#endif					/* OptimizeType */

   return stor;
   }

/*
 * findloops - find both explicit loops and implicit loops caused by
 *  goal-directed evaluation. Allocate stores for them. Determine which
 *  expressions cannot fail (used to eliminate dynamic store allocation
 *  for some bounded expressions). Allocate stores for 'if' and 'case'
 *  expressions that can be resumed. Initialize expression types.
 *  The syntax tree is walked in reverse execution order looking for
 *  failure and for generators.
 */
static int findloops(n, resume, rslt_type)
struct node *n;
int resume;
#ifdef OptimizeType
struct typinfo *rslt_type;
#else					/* OptimizeType */
unsigned int *rslt_type;
#endif					/* OptimizeType */
   {
   struct loop {
      int resume;
      int can_fail;
      int every_cntrl;
#ifdef OptimizeType
      struct typinfo *type;
#else					/* OptimizeType */
      unsigned int *type;
#endif					/* OptimizeType */
      struct loop *prev;
      } loop_info;
   struct loop *loop_sav;
   static struct loop *cur_loop = NULL;
   struct node *cases;
   struct node *clause;
   int can_fail;
   int nargs, i;

   n->store = NULL;
   if (!do_typinfer)
      rslt_type = any_typ;

   switch (n->n_type) {
      case N_Activat:
         if (rslt_type == NULL)
            rslt_type = alloc_typ(n_intrtyp);
         n->type = rslt_type;
         /*
          * Assume activation can fail.
          */
         can_fail = findloops(Tree2(n), 1, NULL);
         can_fail = findloops(Tree1(n), can_fail, NULL);
         n->symtyps = symtyps(2);
         if (optab[Val0(Tree0(n))].tok.t_type == AUGAT)
            n->symtyps->next = symtyps(2);
         break;

      case N_Alt:
         if (rslt_type == NULL)
            rslt_type = alloc_typ(n_intrtyp);
         n->type = rslt_type;

#ifdef TypTrc
         rslt_type = NULL;    /* don't share result loc with subexpressions*/
#endif					/* TypTrc */

         if (resume)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         can_fail = findloops(Tree0(n), resume, rslt_type) |
            findloops(Tree1(n), resume, rslt_type);
         break;

      case N_Apply:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         /* 
          * Assume operation can suspend or fail.
          */
         n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         can_fail = findloops(Tree1(n), 1, NULL);
         can_fail = findloops(Tree0(n), can_fail, NULL);
         n->symtyps = symtyps(max_sym);
         break;

      case N_Augop:
         if (rslt_type == NULL)
            rslt_type = alloc_typ(n_intrtyp);
         n->type = rslt_type;

         can_fail = resume;
         /*
          * Impl0(n) is assignment.
          */
         if (resume && Impl0(n)->ret_flag & DoesSusp)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (MightFail(Impl0(n)->ret_flag))
            can_fail = 1;
         /*
          * Impl1(n) is the augmented operation.
          */
         if (can_fail && Impl1(n)->ret_flag & DoesSusp && n->store == NULL)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (MightFail(Impl1(n)->ret_flag))
            can_fail = 1;
         can_fail = findloops(Tree3(n), can_fail, NULL);  /* operand 2 */
         can_fail = findloops(Tree2(n), can_fail, NULL);  /* operand 1 */
         n->type = Tree2(n)->type;
         Typ4(n) = alloc_typ(n_intrtyp);
         n->symtyps = symtyps(n_arg_sym(Impl1(n)));
         n->symtyps->next = symtyps(n_arg_sym(Impl0(n)));
         break;

      case N_Bar:
         can_fail = findloops(Tree0(n), resume, rslt_type);
         n->type = Tree0(n)->type;
         n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         break;

      case N_Break:
         if (cur_loop == NULL) {
            nfatal(n, "invalid context for break", NULL);
            return 0;
            }
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         loop_sav = cur_loop;
         cur_loop = cur_loop->prev;
         loop_sav->can_fail |= findloops(Tree0(n), loop_sav->resume,
            loop_sav->type);
         cur_loop = loop_sav;
         can_fail = 0;
         break;

      case N_Case:
         if (rslt_type == NULL)
            rslt_type = alloc_typ(n_intrtyp);
         n->type = rslt_type;

#ifdef TypTrc
         rslt_type = NULL;    /* don't share result loc with subexpressions*/
#endif					/* TypTrc */

         if (resume)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);

         /*
          * control clause is bounded
          */
         can_fail = findloops(Tree0(n), 0, NULL);

         cases = Tree1(n);
         while (cases != NULL) {
            if (cases->n_type == N_Ccls) {
               clause = cases;
               cases = NULL;
               }
            else {
               clause = Tree1(cases);
               cases = Tree0(cases);
               }

            /*
             * The expression being compared can be resumed.
             */
            findloops(Tree0(clause), 1, NULL);

            /*
             *  Body.
             */
            can_fail |= findloops(Tree1(clause), resume, rslt_type);
            }

         if (Tree2(n) == NULL)
            can_fail = 1;
         else
            can_fail |= findloops(Tree2(n), resume, rslt_type);  /* default */
         break;

      case N_Create:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         findloops(Tree0(n), 1, NULL);                  /* co-expression code */
        /*
         * precompute type
         */
        i= type_array[coexp_typ].frst_bit;
        if (do_typinfer)
            i += n->new_types[0];
         set_typ(n->type, i);
         can_fail = resume;
         break;

      case N_Cset:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, type_array[cset_typ].frst_bit); /* precompute type */
         can_fail = resume;
         break;

      case N_Empty:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, null_bit); /* precompute type */
         can_fail = resume;
         break;

      case N_Id: {
         struct lentry *var;

         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         /*
          * Precompute type
          */
         var = LSym0(n);
         if (var->flag & F_Global)
            set_typ(n->type, frst_gbl + var->val.global->index);
         else if (var->flag & F_Static)
            set_typ(n->type, frst_gbl + var->val.index);
         else
            set_typ(n->type, frst_loc + var->val.index);
         can_fail = resume;
         }
         break;

      case N_Field:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         can_fail = findloops(Tree0(n), resume, NULL);
         n->symtyps = symtyps(1);
         break;

      case N_If:
         if (rslt_type == NULL)
            rslt_type = alloc_typ(n_intrtyp);
         n->type = rslt_type;

#ifdef TypTrc
         rslt_type = NULL;    /* don't share result loc with subexpressions*/
#endif					/* TypTrc */
         /*
          * control clause is bounded
          */
         findloops(Tree0(n), 0, NULL);
         can_fail = findloops(Tree1(n), resume, rslt_type);
         if (Tree2(n)->n_type == N_Empty)
            can_fail = 1;
         else {
            if (resume)
               n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
            can_fail |= findloops(Tree2(n), resume, rslt_type);
            }
         break;

      case N_Int:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, int_bit); /* precompute type */
         can_fail = resume;
         break;

      case N_Invok:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         nargs = Val0(n);                    /* number of arguments */
         /*
          * Assume operation can suspend and fail.
          */
         if (resume)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         can_fail = 1;
         for (i = nargs; i >= 0; --i)
            can_fail = findloops(n->n_field[i+1].n_ptr, can_fail, NULL);
         n->symtyps = symtyps(max_sym);
         break;

      case N_InvOp:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         nargs = Val0(n);                           /* number of arguments */
         if (resume && Impl1(n)->ret_flag & DoesSusp)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (MightFail(Impl1(n)->ret_flag))
            can_fail = 1;
         else
            can_fail = resume;
         for (i = nargs; i >= 1; --i)
            can_fail = findloops(n->n_field[i+1].n_ptr, can_fail, NULL);
         n->symtyps = symtyps(n_arg_sym(Impl1(n)));
         break;

      case N_InvProc:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         nargs = Val0(n);             /* number of arguments */
         if (resume && Proc1(n)->ret_flag & DoesSusp)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (Proc1(n)->ret_flag & DoesFail)
            can_fail = 1;
         else
            can_fail = resume;
         for (i = nargs; i >= 1; --i)
            can_fail = findloops(n->n_field[i+1].n_ptr, can_fail, NULL);
         break;

      case N_InvRec:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         nargs = Val0(n);                               /* number of args */
         if (err_conv)
            can_fail = 1;
         else
            can_fail = resume;
         for (i = nargs; i >= 1; --i)
            can_fail = findloops(n->n_field[i+1].n_ptr, can_fail, NULL);
         break;

      case N_Limit:
         findloops(Tree0(n), resume, rslt_type);
         can_fail = findloops(Tree1(n), 1, NULL);
         n->type = Tree0(n)->type;
         n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         n->symtyps = symtyps(1);
         break;

      case N_Loop: {
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         loop_info.prev = cur_loop;
         loop_info.resume = resume;
         loop_info.can_fail = 0;
         loop_info.every_cntrl = 0;
         loop_info.type = n->type;
         cur_loop = &loop_info;
         switch ((int)Val0(Tree0(n))) {
            case EVERY:
            case SUSPEND:
               /*
                * The control clause can be resumed. The body is bounded.
                */
               loop_info.every_cntrl = 1;
               can_fail = findloops(Tree1(n), 1, NULL);
               loop_info.every_cntrl = 0;
               findloops(Tree2(n), 0, NULL);
               break;

            case REPEAT:
               /*
                * The loop needs a saved store. The body is bounded.
                */
               findloops(Tree1(n), 0, NULL);
               can_fail = 0;
               break;

            case WHILE:
               /*
                * The loop needs a saved store. The control
                *  clause and the body are each bounded.
                */
               can_fail = findloops(Tree1(n), 0, NULL);
               findloops(Tree2(n), 0, NULL);
               break;

            case UNTIL:
               /*
                * The loop needs a saved store. The control
                *  clause and the body are each bounded.
                */
               findloops(Tree1(n), 0, NULL);
               findloops(Tree2(n), 0, NULL);
               can_fail = 1;
               break;
            }
         n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (do_typinfer && resume)
            n->store->next = alloc_stor(n_gbl + n_loc, n_icntyp);
         can_fail |= cur_loop->can_fail;
         cur_loop = cur_loop->prev;
         }
         break;

      case N_Next:
         if (cur_loop == NULL) {
            nfatal(n, "invalid context for next", NULL);
            return 1;
            }
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         can_fail = cur_loop->every_cntrl;
         break;

      case N_Not:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, null_bit); /* precompute type */
         /*
          * The expression is bounded.
          */
         findloops(Tree0(n), 0, NULL);
         can_fail = 1;
         break;

      case N_Real:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, real_bit); /* precompute type */
         can_fail = resume;
         break;

      case N_Ret:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         if (Val0(Tree0(n)) == RETURN)  {
            /*
             * The expression is bounded.
             */
            findloops(Tree1(n), 0, NULL);
            }
         can_fail = 0;
         break;

      case N_Scan: {
         struct implement *asgn_impl;

         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         n->symtyps = symtyps(1);
         can_fail = resume;
         if (optab[Val0(Tree0(n))].tok.t_type == AUGQMARK) {
            asgn_impl = optab[asgn_loc].binary;
            if (resume && asgn_impl->ret_flag & DoesSusp)
               n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
            if (MightFail(asgn_impl->ret_flag))
               can_fail = 1;
            n->symtyps->next = symtyps(n_arg_sym(asgn_impl));
            }
         can_fail = findloops(Tree2(n), can_fail, NULL);  /* body */
         can_fail = findloops(Tree1(n), can_fail, NULL);  /* subject */ 
         }
         break;

      case N_Sect:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         can_fail = resume;
         /*
          * Impl0(n) is sectioning.
          */
         if (resume && Impl0(n)->ret_flag & DoesSusp)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (MightFail(Impl0(n)->ret_flag))
            can_fail = 1;
         n->symtyps = symtyps(n_arg_sym(Impl0(n)));
         if (Impl1(n) != NULL) {
            /*
             * Impl1(n) is plus or minus
             */
            if (can_fail && Impl1(n)->ret_flag & DoesSusp && n->store == NULL)
               n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
            if (MightFail(Impl1(n)->ret_flag))
               can_fail = 1;
            n->symtyps->next = symtyps(n_arg_sym(Impl1(n)));
            }
         can_fail = findloops(Tree4(n), can_fail, NULL); /* operand 3 */
         can_fail = findloops(Tree3(n), can_fail, NULL); /* operand 2 */
         can_fail = findloops(Tree2(n), can_fail, NULL); /* operand 1 */
         break;

      case N_Slist:
         /*
          * 1st expression is bounded.
          */
         findloops(Tree0(n), 0, NULL);
         can_fail = findloops(Tree1(n), resume, rslt_type);
         n->type = Tree1(n)->type;
         break;

      case N_SmplAsgn:
         can_fail = findloops(Tree3(n), resume, NULL);  /* 2nd operand */
         findloops(Tree2(n), can_fail, rslt_type);      /* variable */
         n->type = Tree2(n)->type;
         break;

      case N_SmplAug:
         can_fail = resume;
         /*
          * Impl1(n) is the augmented operation.
          */
         if (resume && Impl1(n)->ret_flag & DoesSusp)
            n->store = alloc_stor(n_gbl + n_loc, n_icntyp);
         if (MightFail(Impl1(n)->ret_flag))
            can_fail = 1;
         can_fail = findloops(Tree3(n), can_fail, NULL); /* 2nd operand */
         findloops(Tree2(n), can_fail, rslt_type);       /* variable */
         n->symtyps = symtyps(n_arg_sym(Impl1(n)));
         n->type = Tree2(n)->type;
         Typ4(n) = alloc_typ(n_intrtyp);
         break;

      case N_Str:
         if (rslt_type == NULL)
            n->type = alloc_typ(n_intrtyp);
         else
            n->type = rslt_type;
         set_typ(n->type, str_bit); /* precompute type */
         can_fail = resume;
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }
   if (can_fail)
      n->flag = CanFail;
   else
      n->flag = 0;
   return can_fail;
   }

/*
 * symtyps - determine the number of entries needed for a symbol table
 *  that maps argument indexes to types for an operation in the
 *  data base. Allocate the symbol table.
 */
static struct symtyps *symtyps(nsyms)
int nsyms;
   {
   struct symtyps *tab;

   if (nsyms == 0)
      return NULL;

#ifdef OptimizeType
   tab = (struct symtyps *)alloc((unsigned int)(sizeof(struct symtyps) +
      (nsyms - 1) * sizeof(struct typinfo *)));
#else					/* OptimizeType */
   tab = (struct symtyps *)alloc((unsigned int)(sizeof(struct symtyps) +
      (nsyms - 1) * sizeof(int *)));
#endif					/* OptimizeType */
   tab->nsyms = nsyms;
   tab->next = NULL;
   while (nsyms)
      tab->types[--nsyms] = alloc_typ(n_intrtyp);
   return tab;
   }

/*
 * infer_proc - perform type inference on a call to an Icon procedure.
 */
static void infer_prc(proc, n)
struct pentry *proc;
nodeptr n;
   {
   struct store *s_store;
   struct store *f_store;
   struct store *store;
   struct pentry *sv_proc;
   struct t_coexpr *sv_coexp;
   struct lentry *lptr;
   nodeptr n1;
   int i;
   int nparams;
   int coexp_bit;

   /*
    * Determine what co-expressions the procedure might be called from.
    */
   if (cur_coexp == NULL)
      ChkMrgTyp(n_icntyp, cur_proc->coexprs, proc->coexprs)
   else {
      coexp_bit = type_array[coexp_typ].frst_bit + cur_coexp->typ_indx;
      if (!bitset(proc->coexprs, coexp_bit)) {
         ++changed;
         set_typ(proc->coexprs, coexp_bit);
         }
      }

   proc->reachable = 1; /* this procedure can be called */

   /*
    * If this procedure can suspend, there may be backtracking paths
    *  to this invocation. If so, propagate types of globals from the
    *  backtracking paths to the suspends of the procedure and propagate
    *  types of locals to the success store of the call.
    */
   if (proc->ret_flag & DoesSusp && n->store != NULL) {
      for (i = 0; i < n_gbl; ++i)
         ChkMrgTyp(n_icntyp, n->store->types[i], proc->susp_store->types[i])
      for (i = 0; i < n_loc; ++i)
         MrgTyp(n_icntyp, n->store->types[n_gbl + i], succ_store->types[n_gbl +
            i])
      }

   /*
    * Merge the types of global variables into the "in store" of the
    *  procedure. Because the body of the procedure may already have
    *  been processed for this pass, the "changed" flag must be set if
    *  there is a change of type in the store. This will insure that
    *  there will be another iteration in which to propagate the change
    *  into the body.
    */
   store = proc->in_store;
   for (i = 0; i < n_gbl; ++i)
      ChkMrgTyp(n_icntyp, succ_store->types[i], store->types[i])

#ifdef TypTrc
   /*
    * Trace the call.
    */
   if (trcfile != NULL)
      fprintf(trcfile, "%s (%d,%d) %s%s(", n->n_file, n->n_line, n->n_col,
         trc_indent, proc->name);
#endif					/* TypTrc */

   /*
    * Get the types of the arguments, starting with the non-varargs part.
    */
   nparams = proc->nargs;               /* number of parameters */
   if (nparams < 0)
      nparams = -nparams - 1;
   for (i = 0; i < num_args && i < nparams; ++i) {
      typ_deref(arg_typs->types[i], store->types[n_gbl + i], 1);

#ifdef TypTrc
      if (trcfile != NULL) {
         /*
          * Trace the argument type to the call.
          */
         if (i > 0)
            fprintf(trcfile, ", ");
         prt_d_typ(trcfile, arg_typs->types[i]);
         }
#endif					/* TypTrc */

      }

   /*
    * Get the type of the varargs part of the argument list.
    */
   if (proc->nargs < 0)
      while (i < num_args) {
         typ_deref(arg_typs->types[i],
            compnt_array[lst_elem].store->types[proc->arg_lst], 1);

#ifdef TypTrc
         if (trcfile != NULL) {
            /*
             * Trace the argument type to the call.
             */
            if (i > 0)
               fprintf(trcfile, ", ");
            prt_d_typ(trcfile, arg_typs->types[i]);
            }
#endif					/* TypTrc */

         ++i;
         }

   /*
    * Missing arguments have the null type.
    */
   while (i < nparams) {
      set_typ(store->types[n_gbl + i], null_bit);
      ++i;
      }

#ifdef TypTrc
   if (trcfile != NULL)
      fprintf(trcfile, ")\n");
   {
      char *trc_ind_sav = trc_indent;
      trc_indent = "";  /* staring a new procedure, don't indent tracing */
#endif					/* TypTrc */

   /*
    * only perform type inference on the body of a procedure
    *  once per iteration
    */
   if (proc->iteration < iteration) {
      proc->iteration = iteration;
      s_store = succ_store;
      f_store = fail_store;
      sv_proc = cur_proc;
      succ_store = cpy_store(proc->in_store);
      cur_proc = proc;
      sv_coexp = cur_coexp;
      cur_coexp = NULL;     /* we are not in a create expression */
      /*
       * Perform type inference on the initial clause. Static variables
       *  are initialized to null on this path.
       */
      for (lptr = proc->statics; lptr != NULL; lptr = lptr->next)
         set_typ(succ_store->types[lptr->val.index], null_bit);
      n1 = Tree1(proc->tree);
      if (n1->flag & CanFail) {
         /*
          * The initial clause can fail. Because it is bounded, we need
          *  a new failure store that we can merge into the success store
          *  at the end of the clause.
          */
         store = get_store(1);
         fail_store = store;
         infer_nd(n1);
         mrg_store(store, succ_store);
         free_store(store);
         }
      else
         infer_nd(n1);
      /*
       * Perform type inference on the body of procedure. Execution may
       *  pass directly to it without executing initial clause.
       */
      mrg_store(proc->in_store, succ_store);
      n1 = Tree2(proc->tree);
      if (n1->flag & CanFail) {
         /*
          * The body can fail. Because it is bounded, we need a new failure
          *  store that we can merge into the success store at the end of
          *  the procedure.
          */
         store = get_store(1);
         fail_store = store;
         infer_nd(n1);
         mrg_store(store, succ_store);
         free_store(store);
         }
      else
         infer_nd(n1);
      set_ret(NULL);  /* implicit fail */
      free_store(succ_store);
      succ_store = s_store;
      fail_store = f_store;
      cur_proc = sv_proc;
      cur_coexp = sv_coexp;
      }

#ifdef TypTrc
      trc_indent = trc_ind_sav;
   }
#endif					/* TypTrc */

   /*
    * Get updated types for global variables at the end of the call.
    */
   store = proc->out_store;
   for (i = 0; i < n_gbl; ++i)
      CpyTyp(n_icntyp, store->types[i], succ_store->types[i]);

   /*
    * If the procedure can fail, merge variable types into the failure
    *  store.
    */
   if (proc->ret_flag & DoesFail)
      mrg_store(succ_store, fail_store);

   /*
    * The return type of the procedure is the result type of the call.
    */
   MrgTyp(n_intrtyp, proc->ret_typ, n->type);
   }

/*
 * cpy_store - make a copy of a store.
 */
static struct store *cpy_store(source)
struct store *source;
   {
   struct store *dest;
   int stor_sz;
   int i;

   if (source == NULL) 
      dest = get_store(1);
   else {
      stor_sz = n_gbl + n_loc;
      dest = get_store(0);
      for (i = 0; i < stor_sz; ++i)
         CpyTyp(n_icntyp, source->types[i], dest->types[i])
      }
   return dest;
   }

/*
 * mrg_store - merge the source store into the destination store.
 */
static void mrg_store(source, dest)
struct store *source;
struct store *dest;
   {
   int i;

   if (source == NULL)
      return;

   /*
    * Is this store included in the state that must be checked for a fixed
    *  point?
    */
   if (dest->perm) {
      for (i = 0; i < n_gbl + n_loc; ++i)
         ChkMrgTyp(n_icntyp, source->types[i], dest->types[i])
      }
   else {
      for (i = 0; i < n_gbl + n_loc; ++i)
         MrgTyp(n_icntyp, source->types[i], dest->types[i])
      }
   }

/*
 * set_ret - Save return type and the store for global variables.
 */
static void set_ret(typ)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
   {
   int i;

   /*
    * Merge the return type into the type of the procedure, dereferencing
    *  locals in the process.
    */
   if (typ != NULL)
      deref_lcl(typ, cur_proc->ret_typ);

   /*
    * Update the types that variables may have upon exit of the procedure.
    */
   for (i = 0; i < n_gbl; ++i)
      MrgTyp(n_icntyp, succ_store->types[i], cur_proc->out_store->types[i]);
   }

/*
 * deref_lcl - dereference local variable sub-types.
 */
static void deref_lcl(src, dest)
#ifdef OptimizeType
struct typinfo *src;
struct typinfo *dest;
#else					/* OptimizeType */
unsigned int *src;
unsigned int *dest;
#endif					/* OptimizeType */
   {
   int i, j;
   int ref_gbl;
   int frst_stv;
   int num_stv;
   struct store *stv_stor;
   struct type *wktyp;

   /*
    * Make a copy of the type to be dereferenced.
    */
   wktyp = get_wktyp();
   CpyTyp(n_intrtyp, src, wktyp->bits);

   /*
    * Determine which variable types must be dereferenced.  Merge the
    *  dereferenced type into the return type and delete the variable
    *  type. Start with simple local variables.
    */
   for (i = 0; i < n_loc; ++i)
      if (bitset(wktyp->bits, frst_loc + i)) {
         MrgTyp(n_icntyp, succ_store->types[n_gbl + i], wktyp->bits)
         clr_typ(wktyp->bits, frst_loc + i);
         }

   /*
    * Check for substring trapped variables. If a sub-string trapped
    *  variable references a local, add "string" to the return type.
    *  If a sub-string trapped variable references a global, leave the
    *  trapped variable in the return type.
    * It is theoretically possible for a sub-string trapped variable type to
    *  reference both a local and a global. When the trapped variable type
    *  is returned to the calling procedure, the local is re-interpreted
    *  as a local of that procedure. This is a "valid" overestimate of
    *  of the semantics of the return. Because this is unlikely to occur
    *  in real programs, the overestimate is of no practical consequence.
    */
   num_stv = type_array[stv_typ].num_bits;
   frst_stv = type_array[stv_typ].frst_bit;
   stv_stor = compnt_array[str_var].store;
   for (i = 0; i < num_stv; ++i) {
      if (bitset(wktyp->bits, frst_stv + i)) {
         /*
          * We have found substring trapped variable i, see whether it
          *  references locals or globals. Globals include structure
          *  element references.
          */
         for (j = 0; j < n_loc; ++j)
            if (bitset(stv_stor->types[i], frst_loc + j)) {
               set_typ(wktyp->bits, str_bit);
               break;
               }
         ref_gbl = 0;
         for (j = n_icntyp; j < frst_loc; ++j)
            if (bitset(stv_stor->types[i], j)) {
               ref_gbl = 1;
               break;
               }
         /*
          * Keep the trapped variable only if it references globals.
          */
         if (!ref_gbl)
            clr_typ(wktyp->bits, frst_stv + i);
         }
      }

   /*
    * Merge the types into the destination.
    */
   MrgTyp(n_intrtyp, wktyp->bits, dest);

#ifdef TypTrc
   if (trcfile != NULL) {
      prt_typ(trcfile, wktyp->bits);
      fprintf(trcfile, "\n");
      }
#endif					/* TypTrc */

   free_wktyp(wktyp);
   }

/*
 * get_store - get a store large enough to hold globals and locals.
 */
static struct store *get_store(clear)
int clear;
   {
   struct store *store;
   int store_sz;
   int i;

   /*
    * Warning, stores for all procedures must be the same size. In some
    *  situations involving sub-string trapped variables (for example
    *  when using the "default" trapped variable) a referenced local variable
    *  type may be interpreted in a procedure to which it does not belong.
    *  This represents an impossible execution and type inference may
    *  "legally" produce any results for this part of the abstract
    *  interpretation. As long as the store is large enough to include any
    *  such "impossible" variables, type inference will do something legal.
    *  Note that n_loc is the maximum number of locals in any procedure,
    *  so store_sz is large enough.
    */
   store_sz = n_gbl + n_loc;
   if ((store = store_pool) == NULL) {
     store = alloc_stor(store_sz, n_icntyp);
     store->perm = 0;
     }
   else {
      store_pool = store_pool->next;
      /*
       * See if the variables in the store should be initialized to the
       *  empty type.
       */
      if (clear)
         for (i = 0; i < store_sz; ++i)
            ClrTyp(n_icntyp, store->types[i]);
      }
   return store;
   }

static void free_store(store)
struct store *store;
   {
   store->next = store_pool;
   store_pool = store;
   }

/*
 * infer_nd - perform type inference on a subtree of the syntax tree.
 */
static void infer_nd(n)
nodeptr n;
   {
   struct node *cases;
   struct node *clause;
   struct store *s_store;
   struct store *f_store;
   struct store *store;
   struct loop {
      struct store *succ_store;
      struct store *fail_store;
      struct store *next_store;
      struct store *susp_store;
      struct loop *prev;
      } loop_info;
   struct loop *loop_sav;
   static struct loop *cur_loop;
   struct argtyps *sav_argtyp;
   int sav_nargs;
   struct type *wktyp;
   int i;

   switch (n->n_type) {
      case N_Activat:
         infer_act(n);
         break;

      case N_Alt:
         f_store = fail_store;
         store = get_store(1);
         fail_store = store;
         infer_nd(Tree0(n));              /* 1st alternative */

         /*
          * "Correct" type inferencing of alternation has a performance
          *  problem. Propagating stores through nested alternation
          *  requires as many iterations as the depth of the nesting.
          *  This is solved by adding two edges to the flow graph. These
          *  represent impossible execution paths but this does not
          *  affect the soundness of type inferencing and, in "real"
          *  programs, does not affect the preciseness of its inference.
          *  One edge is directly from the 1st alternative to the 2nd.
          *  The other is a backtracking edge immediately back into
          *  the alternation from the 1st alternative.
          */
         mrg_store(succ_store, store); /* imaginary edge to 2nd alternative */

         if (n->store != NULL) {
            mrg_store(succ_store, n->store); /* imaginary backtracking edge */
            mrg_store(n->store, fail_store);
            }
         s_store = succ_store;
         succ_store = store;
         fail_store = f_store;
         infer_nd(Tree1(n));              /* 2nd alternative */
         mrg_store(s_store, succ_store);
         free_store(s_store);
         if (n->store != NULL)
            mrg_store(n->store, fail_store);
         fail_store = n->store;
#ifdef TypTrc
         MrgTyp(n_intrtyp, Tree0(n)->type, n->type);
         MrgTyp(n_intrtyp, Tree1(n)->type, n->type);
#else					/* TypTrc */
         /*
          * Type is computed by sub-expressions directly into n->type.
          */
#endif					/* TypTrc */
         break;

      case N_Apply: {
         struct type *lst_types;
         int frst_lst;
         int num_lst;
         struct store *lstel_stor;

         infer_nd(Tree0(n));          /* thing being invoked */
         infer_nd(Tree1(n));          /* list */

         frst_lst = type_array[list_typ].frst_bit;
         num_lst = type_array[list_typ].num_bits;
         lstel_stor = compnt_array[lst_elem].store;

         /*
          * All that is available is a "summary" of the types of the
          *  elements of the list. Each argument to the invocation
          *  could be any type in the summary. Set up a maximum length
          *  argument list.
          */
         lst_types = get_wktyp();
         typ_deref(Tree1(n)->type, lst_types->bits, 0);
         wktyp = get_wktyp();
         for (i = 0; i < num_lst; ++i)
            if (bitset(lst_types->bits, frst_lst + i))
               MrgTyp(n_icntyp, lstel_stor->types[i], wktyp->bits);
         bitset(wktyp->bits, null_bit); /* arg list extension might be done */

         sav_nargs = num_args;
         sav_argtyp = arg_typs;
         num_args = max_prm;
         arg_typs = get_argtyp();
         for (i = 0; i < max_prm; ++i)
            arg_typs->types[i] = wktyp->bits;
         gen_inv(Tree0(n)->type, n);   /* inference on general invocation */

         free_wktyp(wktyp);
         free_wktyp(lst_types);
         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         }
         break;

      case N_Augop:
         infer_nd(Tree2(n));   /* 1st operand */
         infer_nd(Tree3(n));   /* 2nd operand */
         /*
          * Perform type inference on the operation.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = 2;
         arg_typs->types[0] = Tree2(n)->type;
         arg_typs->types[1] = Tree3(n)->type;
         infer_impl(Impl1(n), n, n->symtyps, Typ4(n));
         chk_succ(Impl1(n)->ret_flag, n->store);
         /*
          * Perform type inference on the assignment.
          */
         arg_typs->types[1] = Typ4(n);
         infer_impl(Impl0(n), n, n->symtyps->next, n->type);
         chk_succ(Impl0(n)->ret_flag, n->store);

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_Bar:
         /*
          * This operation intercepts failure and has an associated
          *  resumption store.  If backtracking reaches this operation
          *  execution may either continue backward or proceed forward
          *  again.
          */ 
         mrg_store(n->store, fail_store);
         mrg_store(n->store, succ_store);
         fail_store = n->store;
         infer_nd(Tree0(n));
         /*
          * Type is computed by operand.
          */
        break;

      case N_Break:
         /*
          * The success and failure stores for the operand of break are
          *  those associated with the enclosing loop.
          */
         fail_store = cur_loop->fail_store;
         loop_sav = cur_loop;
         cur_loop = cur_loop->prev;
         infer_nd(Tree0(n));
         cur_loop = loop_sav;
         mrg_store(succ_store, cur_loop->succ_store);
         if (cur_loop->susp_store != NULL)
            mrg_store(cur_loop->susp_store, fail_store);
         free_store(succ_store);
         succ_store = get_store(1);  /* empty store says: can't get past here */
         fail_store = dummy_stor;    /* shouldn't be used */
         /*
          * Result of break is empty type. Result type of expression
          *  is computed directly into result type of loop.
          */
         break;

      case N_Case:
         f_store = fail_store;
         s_store = get_store(1);
         infer_nd(Tree0(n));     /* control clause */
         cases = Tree1(n);
         while (cases != NULL) {
            if (cases->n_type == N_Ccls) {
               clause = cases;
               cases = NULL;
               }
            else {
               clause = Tree1(cases);
               cases = Tree0(cases);
               }

            /*
             * Set up a failure store to capture the effects of failure
             *  of the selection clause.
             */
            store = get_store(1);
            fail_store = store;
            infer_nd(Tree0(clause));             /* value of clause */

            /*
             * Create the effect of the possible failure of the comparison
             *  of the selection value to the control value.
             */
            mrg_store(succ_store, fail_store);

            /*
             * The success and failure stores and the result of the body
             *  of the clause are those of the whole case expression.
             */
            fail_store = f_store;
            infer_nd(Tree1(clause));             /* body of clause */
            mrg_store(succ_store, s_store);
            free_store(succ_store);
            succ_store = store;
            if (n->store != NULL)
               mrg_store(n->store, fail_store);  /* 'case' can be resumed */
#ifdef TypTrc
            MrgTyp(n_intrtyp, Tree1(clause)->type, n->type);
#else					/* TypTrc */
            /*
             * Type is computed by case clause directly into n->type.
            */
#endif					/* TypTrc */
            }

         /*
          * Check for default clause.
          */
         if (Tree2(n) == NULL)
            mrg_store(succ_store, f_store);
         else {
            fail_store = f_store;
            infer_nd(Tree2(n));                  /* default */
            mrg_store(succ_store, s_store);
            if (n->store != NULL)
               mrg_store(n->store, fail_store);  /* 'case' can be resumed */
#ifdef TypTrc
            MrgTyp(n_intrtyp, Tree2(n)->type, n->type);
#else					/* TypTrc */
            /*
             * Type is computed by default clause directly into n->type.
             */
#endif					/* TypTrc */
            }
         free_store(succ_store);
         succ_store = s_store;
         if (n->store != NULL)
            fail_store = n->store;
         break;

      case N_Create:
         /*
          * Record initial values of local variables for coexpression.
          */
         store = coexp_map[n->new_types[0]]->in_store;
         for (i = 0; i < n_loc; ++i)
            ChkMrgTyp(n_icntyp, succ_store->types[n_gbl + i],
               store->types[n_gbl + i])
         /*
          * Type is precomputed.
          */
         break;

      case N_Cset:
      case N_Empty:
      case N_Id:
      case N_Int:
      case N_Real:
      case N_Str:
         /*
          * Type is precomputed.
          */
         break;

      case N_Field: {
         struct fentry *fp;
         struct par_rec *rp;
         int frst_rec;

         if ((fp = flookup(Str0(Tree1(n)))) == NULL) {
            break;  /* error message printed elsewhere */
            }

         /*
          * Determine the record types.
          */
         infer_nd(Tree0(n));
         typ_deref(Tree0(n)->type, n->symtyps->types[0], 0);

         /*
          * For each record containing this field, get the tupe of
          *  the field in that record.
          */
         frst_rec = type_array[rec_typ].frst_bit;
         for (rp = fp->rlist; rp != NULL; rp = rp->next) {
            if (bitset(n->symtyps->types[0], frst_rec + rp->rec->rec_num))
               set_typ(n->type, frst_fld + rp->rec->frst_fld + rp->offset);
            }
         }
         break;

      case N_If:
         f_store = fail_store;
         if (Tree2(n)->n_type != N_Empty) {
            /*
             * If there is an else clause, we must set up a failure store
             *  to capture the effects of failure of the control clause.
             */
            store = get_store(1);
            fail_store = store;
            }

         infer_nd(Tree0(n));           /* control clause */

         /*
          * If the control clause succeeds, execution passes into the
          *  then clause with the failure store for the entire if expression.
          */
         fail_store = f_store;
         infer_nd(Tree1(n));           /* then clause */

         if (Tree2(n)->n_type != N_Empty) {
            if (n->store != NULL)
               mrg_store(n->store, fail_store); /* 'if' expr can be resumed */
            s_store = succ_store;

            /*
             * The entering success store of the else clause is the failure
             *  store of the control clause. The failure store is that of
             *  the entire if expression.
             */
            succ_store = store;
            fail_store = f_store;
            infer_nd(Tree2(n));        /* else clause */

            if (n->store != NULL) {
               mrg_store(n->store, fail_store); /* 'if' expr can be resumed */
               fail_store = n->store;
               }

            /*
             * Join the exiting success stores of the then and else clauses.
             */
            mrg_store(s_store, succ_store);
            free_store(s_store);
            }

#ifdef TypTrc
         MrgTyp(n_intrtyp, Tree1(n)->type, n->type);
         if (Tree2(n)->n_type != N_Empty)
            MrgTyp(n_intrtyp, Tree2(n)->type, n->type);
#else					/* TypTrc */
         /*
          * Type computed by 'then' and 'else' clauses directly into n->type.
          */
#endif					/* TypTrc */
         break;

      case N_Invok:
         /*
          * General invocation.
          */
         infer_nd(Tree1(n));          /* thing being invoked */

         /*
          * Perform type inference on all the arguments and copy the
          *  results into the argument type array.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = Val0(n);          /* number of arguments */
         for (i = 0; i < num_args; ++i) {
            infer_nd(n->n_field[i+2].n_ptr);           /* arg i */
            arg_typs->types[i] = n->n_field[i+2].n_ptr->type;
            }

         /*
          * If this is mutual evaluation, get the type of the last argument,
          *  otherwise do inference on general invocation.
          */
         if (Tree1(n)->n_type == N_Empty) {
            MrgTyp(n_intrtyp, arg_typs->types[num_args - 1], n->type);
            }
         else
            gen_inv(Tree1(n)->type, n);

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_InvOp:
         /*
          * Invocation of a run-time operation. Perform inference on all
          *  the arguments, copying the results into the argument type
          *  array.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = Val0(n);                          /* number of arguments */
         for (i = 0; i < num_args; ++i) {
            infer_nd(n->n_field[i+2].n_ptr);           /* arg i */
            arg_typs->types[i] = n->n_field[i+2].n_ptr->type;
            }

         /*
          * Perform inference on operation invocation.
          */
         infer_impl(Impl1(n), n, n->symtyps, n->type);
         chk_succ(Impl1(n)->ret_flag, n->store);

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_InvProc:
         /*
          * Invocation of a procedure. Perform inference on all
          *  the arguments, copying the results into the argument type
          *  array.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = Val0(n);                          /* number of arguments */
         for (i = 0; i < num_args; ++i) {
            infer_nd(n->n_field[i+2].n_ptr);           /* arg i */
            arg_typs->types[i] = n->n_field[i+2].n_ptr->type;
            }

         /*
          * Perform inference on the procedure invocation.
          */
         infer_prc(Proc1(n), n);
         chk_succ(Proc1(n)->ret_flag, n->store);

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_InvRec:
         /*
          * Invocation of a record constructor. Perform inference on all
          *  the arguments, copying the results into the argument type
          *  array.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = Val0(n);                          /* number of arguments */
         for (i = 0; i < num_args; ++i) {
            infer_nd(n->n_field[i+2].n_ptr);           /* arg i */
            arg_typs->types[i] = n->n_field[i+2].n_ptr->type;
            }

         infer_con(Rec1(n), n); /* inference on constructor invocation */

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_Limit:
         infer_nd(Tree1(n));                 /* limit */
         typ_deref(Tree1(n)->type, n->symtyps->types[0], 0);
         mrg_store(succ_store, fail_store);  /* limit might be 0 */
         mrg_store(n->store, fail_store);    /* resumption may bypass expr */
         infer_nd(Tree0(n));                 /* expression */
         if (fail_store != NULL)
            mrg_store(n->store, fail_store); /* expression may be resumed */
         fail_store = n->store;
         /*
          * Type is computed by expression being limited.
          */
         break;

      case N_Loop: {
         /*
          * Establish stores used by break and next.
          */
         loop_info.prev = cur_loop;
         loop_info.succ_store = get_store(1);
         loop_info.fail_store = fail_store;
         loop_info.next_store = NULL;
         loop_info.susp_store = n->store->next;
         cur_loop = &loop_info;

         switch ((int)Val0(Tree0(n))) {
            case EVERY:
               infer_nd(Tree1(n));              /* control clause */
               f_store = fail_store;

               /*
                * Next in the do clause resumes the control clause as
                *  does success of the do clause.
                */
               loop_info.next_store = fail_store;
               infer_nd(Tree2(n));              /* do clause  */
               mrg_store(succ_store, f_store);
               break;

            case REPEAT:
               /*
                * The body of the loop can be entered by entering the
                *  loop, by executing a next in the body, or by having
                *  the loop succeed or fail. n->store captures all but
                *  the first case, which is covered by the initial success
                *  store.
                */
               fail_store = n->store;
               mrg_store(n->store, succ_store);
               loop_info.next_store = n->store;
               infer_nd(Tree1(n));
               mrg_store(succ_store, n->store);
               break;

            case SUSPEND:
               infer_nd(Tree1(n));              /* value */
#ifdef TypTrc
              if (trcfile != NULL)
                  fprintf(trcfile, "%s (%d,%d) suspend ", n->n_file, n->n_line,
                     n->n_col);
#endif					/* TypTrc */

               set_ret(Tree1(n)->type); /* set return type of procedure */

               /*
                * Get changes to types of global variables from
                *  resumption.
                */
               store = cur_proc->susp_store;
               for (i = 0; i < n_gbl; ++i)
                  CpyTyp(n_icntyp, store->types[i], succ_store->types[i]);

               /*
                * Next in the do clause resumes the control clause as
                *  does success of the do clause.
                */
               f_store = fail_store;
               loop_info.next_store = fail_store;
               infer_nd(Tree2(n));              /* do clause  */
               mrg_store(succ_store, f_store);
               break;

            case WHILE:
               /*
                * The control clause can be entered by entering the loop,
                *  executing a next expression, or by having the do clause
                *  succeed or fail. n->store captures all but the first case,
                *  which is covered by the initial success store.
                */
               mrg_store(n->store, succ_store);
               loop_info.next_store = n->store;
               infer_nd(Tree1(n));              /* control clause */
               fail_store = n->store;
               infer_nd(Tree2(n));              /* do clause  */
               mrg_store(succ_store, n->store);
               break;

            case UNTIL:
               /*
                * The control clause can be entered by entering the loop,
                *  executing a next expression, or by having the do clause
                *  succeed or fail. n->store captures all but the first case,
                *  which is covered by the initial success store.
                */
               mrg_store(n->store, succ_store);
               loop_info.next_store = n->store;
               f_store = fail_store;
               /*
                * Set up a failure store to capture the effects of failure
                *  of the control clause.
                */
               store = get_store(1);
               fail_store = store;
               infer_nd(Tree1(n));              /* control clause */
               mrg_store(succ_store, f_store);
               free_store(succ_store);
               succ_store = store;
               fail_store = n->store;
               infer_nd(Tree2(n));              /* do clause  */
               mrg_store(succ_store, n->store);
               break;
            }
         free_store(succ_store);
         succ_store = loop_info.succ_store;
         if (n->store->next != NULL)
             fail_store = n->store->next;
         cur_loop = cur_loop->prev;
         /*
          * Type is computed by break expressions.
          */
         }
         break;

      case N_Next:
         if (cur_loop->next_store == NULL)
            mrg_store(succ_store, fail_store);   /* control clause of every */
         else
            mrg_store(succ_store, cur_loop->next_store);
         free_store(succ_store);
         succ_store = get_store(1);  /* empty store says: can't get past here */
         fail_store = dummy_stor;    /* shouldn't be used */
         /*
          * Result is empty type.
          */
         break;

      case N_Not:
         /*
          * Set up a failure store to capture the effects of failure
          *  of the negated expression, it becomes the success store
          *  of the entire expression.
          */
         f_store = fail_store;
         store = get_store(1);
         fail_store = store;
         infer_nd(Tree0(n));
         mrg_store(succ_store, f_store); /* if success, then fail */
         free_store(succ_store);
         succ_store = store;
         fail_store = f_store;
         /*
          * Type is precomputed.
          */
         break;

      case N_Ret:
         if (Val0(Tree0(n)) == RETURN) {
            if (Tree1(n)->flag & CanFail) {
               /*
                * Set up a failure store to capture the effects of failure
                *  of the returned expression and the corresponding procedure
                *  failure.
                */
               store = get_store(1);
               fail_store = store;
               infer_nd(Tree1(n));    /* return value */
               mrg_store(store, succ_store);
               free_store(store);
               }
            else
               infer_nd(Tree1(n));    /* return value */

#ifdef TypTrc
           if (trcfile != NULL)
               fprintf(trcfile, "%s (%d,%d) return ", n->n_file, n->n_line,
               n->n_col);
#endif					/* TypTrc */

            set_ret(Tree1(n)->type);
            }
         else {  /* fail */
            set_ret(NULL);

#ifdef TypTrc
           if (trcfile != NULL) 
               fprintf(trcfile, "%s (%d,%d) fail\n", n->n_file, n->n_line,
               n->n_col);
#endif					/* TypTrc */

            }
         free_store(succ_store);
         succ_store = get_store(1);  /* empty store says: can't get past here */
         fail_store = dummy_stor;    /* shouldn't be used */
         /*
          * Empty type.
          */
         break;

      case N_Scan: {
         struct implement *asgn_impl;

         infer_nd(Tree1(n));   /* subject */ 
         typ_deref(Tree1(n)->type, n->symtyps->types[0], 0);
         infer_nd(Tree2(n));   /* body */

         if (optab[Val0(Tree0(n))].tok.t_type == AUGQMARK) {
            /*
             * Perform type inference on the assignment.
             */
            asgn_impl = optab[asgn_loc].binary;
            sav_argtyp = arg_typs;
            sav_nargs = num_args;
            arg_typs = get_argtyp();
            num_args = 2;
            arg_typs->types[0] = Tree1(n)->type;
            arg_typs->types[1] = Tree2(n)->type;
            infer_impl(asgn_impl, n, n->symtyps->next, n->type);
            chk_succ(asgn_impl->ret_flag, n->store);
            free_argtyp(arg_typs);
            arg_typs = sav_argtyp;
            num_args = sav_nargs;
            }
         else
            MrgTyp(n_intrtyp, Tree2(n)->type, n->type);
         }
         break;

      case N_Sect:
         infer_nd(Tree2(n));     /* 1st operand */
         infer_nd(Tree3(n));     /* 2nd operand */
         infer_nd(Tree4(n));     /* 3rd operand */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         if (Impl1(n) != NULL) {
            /*
             * plus or minus.
             */
            num_args = 2;
            arg_typs->types[0] = Tree3(n)->type;
            arg_typs->types[1] = Tree4(n)->type;
            wktyp = get_wktyp();
            infer_impl(Impl1(n), n, n->symtyps->next, wktyp->bits);
            chk_succ(Impl1(n)->ret_flag, n->store);
            arg_typs->types[2] = wktyp->bits;
            }
         else
            arg_typs->types[2] = Tree4(n)->type;
         num_args = 3;
         arg_typs->types[0] = Tree2(n)->type;
         arg_typs->types[1] = Tree3(n)->type;
         /*
          * sectioning 
          */
         infer_impl(Impl0(n), n, n->symtyps, n->type);
         chk_succ(Impl0(n)->ret_flag, n->store);
         if (Impl1(n) != NULL)
           free_wktyp(wktyp);
         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;
         break;

      case N_Slist:
         f_store = fail_store;
         if (Tree0(n)->flag & CanFail) {
            /*
             * Set up a failure store to capture the effects of failure
             *  of the first operand; this is merged into the
             *  incoming success store of the second operand.
             */
            store = get_store(1);
            fail_store = store;
            infer_nd(Tree0(n));
            mrg_store(store, succ_store);
            free_store(store);
            }
         else
            infer_nd(Tree0(n));
         fail_store = f_store;
         infer_nd(Tree1(n));
         /*
          * Type is computed by second operand.
          */
         break;

      case N_SmplAsgn: {
         /*
          * Optimized assignment to a named variable.
          */
         struct lentry *var;
         int indx;

         infer_nd(Tree3(n));
         var = LSym0(Tree2(n));
         if (var->flag & F_Global)
            indx = var->val.global->index;
         else if (var->flag & F_Static)
            indx = var->val.index;
         else
            indx = n_gbl + var->val.index;
         ClrTyp(n_icntyp, succ_store->types[indx]);
         typ_deref(Tree3(n)->type, succ_store->types[indx], 0);

#ifdef TypTrc
         /*
          * Trace assignment.
          */
         if (trcfile != NULL) {
            fprintf(trcfile, "%s (%d,%d) %s%s := ", n->n_file, n->n_line,
               n->n_col, trc_indent, var->name);
            prt_d_typ(trcfile, Tree3(n)->type);
            fprintf(trcfile, "\n");
            }
#endif					/* TypTrc */
         /*
          * Type is precomputed.
          */
         }
         break;

      case N_SmplAug: {
         /*
          * Optimized augmented assignment to a named variable.
          */
         struct lentry *var;
         int indx;

         /*
          * Perform type inference on the operation.
          */
         infer_nd(Tree3(n));            /* 2nd operand */

         /*
          * Set up type array for arguments of operation.
          */
         sav_argtyp = arg_typs;
         sav_nargs = num_args;
         arg_typs = get_argtyp();
         num_args = 2;
         arg_typs->types[0] = Tree2(n)->type;  /* type was precomputed */
         arg_typs->types[1] = Tree3(n)->type;

         /*
          * Perform inference on the operation.
          */
         infer_impl(Impl1(n), n, n->symtyps, Typ4(n));
         chk_succ(Impl1(n)->ret_flag, n->store);

         /*
          * Perform assignment to the variable.
          */
         var = LSym0(Tree2(n));
         if (var->flag & F_Global)
            indx = var->val.global->index;
         else if (var->flag & F_Static)
            indx = var->val.index;
         else
            indx = n_gbl + var->val.index;
         ClrTyp(n_icntyp, succ_store->types[indx]);
         typ_deref(Typ4(n), succ_store->types[indx], 0);

#ifdef TypTrc
         /*
          * Trace assignment.
          */
         if (trcfile != NULL) {
            fprintf(trcfile, "%s (%d,%d) %s%s := ", n->n_file, n->n_line,
               n->n_col, trc_indent, var->name);
            prt_d_typ(trcfile, Typ4(n));
            fprintf(trcfile, "\n");
            }
#endif					/* TypTrc */

         free_argtyp(arg_typs);
         arg_typs = sav_argtyp;
         num_args = sav_nargs;

         /*
          * Type is precomputed.
          */
         }
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }
   }

/*
 * infer_con - perform type inference for the invocation of a record
 *  constructor.
 */
static void infer_con(rec, n)
struct rentry *rec;
nodeptr n;
   {
   int fld_indx;
   int nfields;
   int i;

#ifdef TypTrc
   if (trcfile != NULL)
      fprintf(trcfile, "%s (%d,%d) %s%s(", n->n_file, n->n_line, n->n_col,
         trc_indent, rec->name);
#endif					/* TypTrc */

   /*
    * Dereference argument types into appropriate entries of field store.
    */
   fld_indx = rec->frst_fld;
   nfields = rec->nfields;
   for (i = 0; i < num_args && i < nfields; ++i) {
      typ_deref(arg_typs->types[i], fld_stor->types[fld_indx++], 1);

#ifdef TypTrc
      if (trcfile != NULL) {
         if (i > 0)
            fprintf(trcfile, ", ");
         prt_d_typ(trcfile, arg_typs->types[i]);
         }
#endif					/* TypTrc */

      }

   /*
    * If there are too few arguments, add null type to appropriate entries
    *  of field store.
    */
   while (i < nfields) {
      if (!bitset(fld_stor->types[fld_indx], null_bit)) {
         ++changed;
         set_typ(fld_stor->types[fld_indx], null_bit);
         }
      ++fld_indx;
      ++i;
      }

   /*
    * return record type
    */
   set_typ(n->type, type_array[rec_typ].frst_bit + rec->rec_num);

#ifdef TypTrc
   if (trcfile != NULL) {
      fprintf(trcfile, ")  =>>  ");
      prt_typ(trcfile, n->type);
      fprintf(trcfile, "\n");
      }
#endif					/* TypTrc */
   }

/*
 * infer_act - perform type inference on coexpression activation.
 */
static void infer_act(n)
nodeptr n;
   {
   struct implement *asgn_impl;
   struct store *s_store;
   struct store *f_store;
   struct store *e_store;
   struct store *store;
   struct t_coexpr *sv_coexp;
   struct t_coexpr *coexp;
   struct type *rslt_typ;
   struct argtyps *sav_argtyp;
   int frst_coexp;
   int num_coexp;
   int sav_nargs;
   int i;
   int j;

#ifdef TypTrc
   FILE *trc_save;
#endif					/* TypTrc */

   num_coexp = type_array[coexp_typ].num_bits;
   frst_coexp = type_array[coexp_typ].frst_bit;

   infer_nd(Tree1(n));   /* value to transmit */ 
   infer_nd(Tree2(n));   /* coexpression */

   /*
    * Dereference the two arguments. Note that only locals in the
    *  transmitted value are dereferenced.
    */

#ifdef TypTrc
   trc_save = trcfile;
   trcfile = NULL;  /* don't trace value during dereferencing */
#endif					/* TypTrc */

   deref_lcl(Tree1(n)->type, n->symtyps->types[0]);

#ifdef TypTrc
   trcfile = trc_save;
#endif					/* TypTrc */

   typ_deref(Tree2(n)->type, n->symtyps->types[1], 0);

   rslt_typ = get_wktyp();

   /*
    * Set up a store for the end of the activation and propagate local
    *  variables across the activation; the activation may succeed or
    *  fail.
    */
   e_store = get_store(1);
   for (i = 0; i < n_loc; ++i)
      CpyTyp(n_icntyp, succ_store->types[n_gbl + i], e_store->types[n_gbl + i])
   if (fail_store->perm) {
      for (i = 0; i < n_loc; ++i)
         ChkMrgTyp(n_icntyp, succ_store->types[n_gbl + i],
            fail_store->types[n_gbl + i])
         }
    else {
      for (i = 0; i < n_loc; ++i)
         MrgTyp(n_icntyp, succ_store->types[n_gbl + i],
            fail_store->types[n_gbl + i])
      }


   /*
    * Go through all the co-expressions that might be activated,
    *  perform type inference on them, and transmit stores along
    *  the execution paths induced by the activation.
    */
   s_store = succ_store;
   f_store = fail_store;
   for (j = 0; j < num_coexp; ++j) {
      if (bitset(n->symtyps->types[1], frst_coexp + j)) {
         coexp = coexp_map[j];
         /*
          * Merge the types of global variables into the "in store" of the
          *  co-expression. Because the body of the co-expression may already
          *  have been processed for this pass, the "changed" flag must be
          *  set if there is a change of type in the store. This will insure
          *  that there will be another iteration in which to propagate the
          *  change into the body.
          */
         store = coexp->in_store;
         for (i = 0; i < n_gbl; ++i)
            ChkMrgTyp(n_icntyp, s_store->types[i], store->types[i])

         ChkMrgTyp(n_intrtyp, n->symtyps->types[0], coexp->act_typ)

         /*
          * Only perform type inference on the body of a co-expression
          *  once per iteration. The main co-expression has no body.
          */
         if (coexp->iteration < iteration & coexp->n != NULL) {
            coexp->iteration = iteration;
            succ_store = cpy_store(coexp->in_store);
            fail_store = coexp->out_store;
            sv_coexp = cur_coexp;
            cur_coexp = coexp;
            infer_nd(coexp->n);

            /*
             * Dereference the locals in the value resulting from
             *  the execution of the co-expression body.
             */

#ifdef TypTrc
            if (trcfile != NULL)
               fprintf(trcfile, "%s (%d,%d) %sC%d  =>>  ", coexp->n->n_file,
                  coexp->n->n_line, coexp->n->n_col, trc_indent, j);
#endif					/* TypTrc */

            deref_lcl(coexp->n->type, coexp->rslt_typ);

            mrg_store(succ_store, coexp->out_store);
            free_store(succ_store);
            cur_coexp = sv_coexp;
            }

         /*
          * Get updated types for global variables, assuming the co-expression
          *  fails or returns by completing.
          */
         store = coexp->out_store;
         for (i = 0; i < n_gbl; ++i)
            MrgTyp(n_icntyp, store->types[i], e_store->types[i]);
         if (f_store->perm) {
            for (i = 0; i < n_gbl; ++i)
               ChkMrgTyp(n_icntyp, store->types[i], f_store->types[i]);
            }
          else {
            for (i = 0; i < n_gbl; ++i)
               MrgTyp(n_icntyp, store->types[i], f_store->types[i]);
            }
         MrgTyp(n_intrtyp, coexp->rslt_typ, rslt_typ->bits)
         }
      }

   /*
    * Control may return from the activation if another co-expression
    *  activates the current one. If we are in a create expression,
    *  cur_coexp is the current co-expression, otherwise the current
    *  procedure may be called within several co-expressions.
    */
   if (cur_coexp == NULL) {
      for (j = 0; j < num_coexp; ++j)
         if (bitset(cur_proc->coexprs, frst_coexp + j))
            mrg_act(coexp_map[j], e_store, rslt_typ);
      }
   else
      mrg_act(cur_coexp, e_store, rslt_typ);

   free_store(s_store);
   succ_store = e_store;
   fail_store = f_store;


#ifdef TypTrc
   if (trcfile != NULL) {
      fprintf(trcfile, "%s (%d,%d) %s", n->n_file, n->n_line, n->n_col,
         trc_indent);
      prt_typ(trcfile, n->symtyps->types[0]);
      fprintf(trcfile, " @ ");
      prt_typ(trcfile, n->symtyps->types[1]);
      fprintf(trcfile, "  =>>  ");
      prt_typ(trcfile, rslt_typ->bits);
      fprintf(trcfile, "\n");
      }
#endif					/* TypTrc */

   if (optab[Val0(Tree0(n))].tok.t_type == AUGAT) {
      /*
       * Perform type inference on the assignment.
       */
      asgn_impl = optab[asgn_loc].binary;
      sav_argtyp = arg_typs;
      sav_nargs = num_args;
      arg_typs = get_argtyp();
      num_args = 2;
      arg_typs->types[0] = Tree1(n)->type;
      arg_typs->types[1] = rslt_typ->bits;
      infer_impl(asgn_impl, n, n->symtyps->next, n->type);
      chk_succ(asgn_impl->ret_flag, n->store);
      free_argtyp(arg_typs);
      arg_typs = sav_argtyp;
      num_args = sav_nargs;
      }
   else
      ChkMrgTyp(n_intrtyp, rslt_typ->bits, n->type)

   free_wktyp(rslt_typ);
   }

/*
 * mrg_act - merge entry information for the co-expression to the
 *  the ending store and result type for the activation being
 *  analyzed.
 */
static void mrg_act(coexp, e_store, rslt_typ)
struct t_coexpr *coexp;
struct store *e_store;
struct type *rslt_typ;
   {
   struct store *store;
   int i;

   store = coexp->in_store;
   for (i = 0; i < n_gbl; ++i)
      MrgTyp(n_icntyp, store->types[i], e_store->types[i]);

   MrgTyp(n_intrtyp, coexp->act_typ, rslt_typ->bits)
   }

/*
 * typ_deref - perform dereferencing in the abstract type realm.
 */
static void typ_deref(src, dest, chk)
#ifdef OptimizeType
struct typinfo *src;
struct typinfo *dest;
#else					/* OptimizeType */
unsigned int *src;
unsigned int *dest;
#endif					/* OptimizeType */
int chk;
   {
   struct store *tblel_stor;
   struct store *tbldf_stor;
   struct store *ttv_stor;
   struct store *store;
   unsigned int old;
   int num_tbl;
   int frst_tbl;
   int num_bits;
   int frst_bit;
   int i;
   int j;
   int ret;
/*
   if (src->bits == NULL) {
      src->bits = alloc_mem_typ(src->size);
      xfer_packed_types(src);
   }
   if (dest->bits == NULL) {
      dest->bits = alloc_mem_typ(dest->size);
      xfer_packed_types(dest);
   }
*/
   /*
    * copy values to destination
    */
#ifdef OptimizeType
   if ((src->bits != NULL) && (dest->bits != NULL))  {
       for (i = 0; i < NumInts(n_icntyp) - 1; ++i) {
          old = dest->bits[i];
          dest->bits[i] |= src->bits[i];
          if (chk && (old != dest->bits[i]))
             ++changed;
       }
       old = dest->bits[i];
       dest->bits[i] |= src->bits[i] & val_mask;  /* mask out variables */
       if (chk && (old != dest->bits[i]))
          ++changed;
    }
    else if ((src->bits != NULL) && (dest->bits == NULL)) {
       dest->bits = alloc_mem_typ(DecodeSize(dest->packed));
       xfer_packed_types(dest);
       for (i = 0; i < NumInts(n_icntyp) - 1; ++i) {
          old = dest->bits[i];
          dest->bits[i] |= src->bits[i];
          if (chk && (old != dest->bits[i]))
             ++changed;
          }
       old = dest->bits[i];
       dest->bits[i] |= src->bits[i] & val_mask;  /* mask out variables */
       if (chk && (old != dest->bits[i]))
          ++changed;
    }  
    else if ((src->bits == NULL) && (dest->bits != NULL))  {
       ret = xfer_packed_to_bits(src, dest, n_icntyp);       
       if (chk)
          changed += ret;
    }
    else {
       ret = mrg_packed_to_packed(src, dest, n_icntyp);
       if (chk)
          changed += ret;
    }
#else					/* OptimizeType */
   for (i = 0; i < NumInts(n_icntyp) - 1; ++i) {
      old = dest[i];
      dest[i] |= src[i];
      if (chk && (old != dest[i]))
         ++changed;
      }
   old = dest[i];
   dest[i] |= src[i] & val_mask;  /* mask out variables */
   if (chk && (old != dest[i]))
      ++changed;
#endif					/* OptimizeType */

   /* 
    * predefined variables whose types do not change.
    */
   for (i = 0; i < num_typs; ++i) {
      if (icontypes[i].deref == DrfCnst) {
         if (bitset(src, type_array[i].frst_bit))
            if (chk)
               ChkMrgTyp(n_icntyp, type_array[i].typ, dest)
            else
               MrgTyp(n_icntyp, type_array[i].typ, dest)
         }
      }


   /*
    * substring trapped variables
    */
   num_bits = type_array[stv_typ].num_bits;
   frst_bit = type_array[stv_typ].frst_bit;
   for (i = 0; i < num_bits; ++i)
      if (bitset(src, frst_bit + i))
         if (!bitset(dest, str_bit)) {
            if (chk)
               ++changed;
            set_typ(dest, str_bit);
            }

   /*
    * table element trapped variables
    */
   num_bits = type_array[ttv_typ].num_bits;
   frst_bit = type_array[ttv_typ].frst_bit;
   num_tbl = type_array[tbl_typ].num_bits;
   frst_tbl = type_array[tbl_typ].frst_bit;
   tblel_stor = compnt_array[tbl_val].store;
   tbldf_stor = compnt_array[tbl_dflt].store;
   ttv_stor = compnt_array[trpd_tbl].store;
   for (i = 0; i < num_bits; ++i)
      if (bitset(src, frst_bit + i))
         for (j = 0; j < num_tbl; ++j)
             if (bitset(ttv_stor->types[i], frst_tbl + j)) {
                if (chk) {
                   ChkMrgTyp(n_icntyp, tblel_stor->types[j], dest)
                   ChkMrgTyp(n_icntyp, tbldf_stor->types[j], dest)
                   }
                else {
                   MrgTyp(n_icntyp, tblel_stor->types[j], dest)
                   MrgTyp(n_icntyp, tbldf_stor->types[j], dest)
                   }
                }

   /*
    * Aggregate compontents that are variables.
    */
   for (i = 0; i < num_cmpnts; ++i) {
      if (typecompnt[i].var) {
         frst_bit = compnt_array[i].frst_bit;
         num_bits = compnt_array[i].num_bits;
         store = compnt_array[i].store;
         for (j = 0; j < num_bits; ++j) {
            if (bitset(src, frst_bit + j))
               if (chk)
                  ChkMrgTyp(n_icntyp, store->types[j], dest)
               else
                  MrgTyp(n_icntyp, store->types[j], dest)
            }
         }
      }


   /*
    * record fields
    */
   for (i = 0; i < n_fld; ++i)
      if (bitset(src, frst_fld + i)) {
         if (chk)
            ChkMrgTyp(n_icntyp, fld_stor->types[i], dest)
         else
            MrgTyp(n_icntyp, fld_stor->types[i], dest)
      }

   /*
    * global variables
    */
   for (i = 0; i < n_gbl; ++i)
      if (bitset(src, frst_gbl + i)) {
         if (chk)
            ChkMrgTyp(n_icntyp, succ_store->types[i], dest)
         else
            MrgTyp(n_icntyp, succ_store->types[i], dest)
      }

   /*
    * local variables
    */
   for (i = 0; i < n_loc; ++i)
      if (bitset(src, frst_loc + i)) {
         if (chk)
            ChkMrgTyp(n_icntyp, succ_store->types[n_gbl + i], dest)
         else
            MrgTyp(n_icntyp, succ_store->types[n_gbl + i], dest)
      }
}

/*
 * infer_impl - perform type inference on a call to built-in operation
 *   using the implementation entry from the data base.
 */
static void infer_impl(impl, n, symtyps, rslt_typ)
struct implement *impl;
nodeptr n;
struct symtyps *symtyps;
#ifdef OptimizeType
struct typinfo *rslt_typ;
#else					/* OptimizeType */
unsigned int *rslt_typ;
#endif					/* OptimizeType */
   {
#ifdef OptimizeType
   struct typinfo *typ;
#else					/* OptimizeType */
   unsigned int *typ;
#endif					/* OptimizeType */
   int flag;
   int nparms;
   int i;
   int j; 

#ifdef TypTrc
   if (trcfile != NULL) {
      fprintf(trcfile, "%s (%d,%d) %s", n->n_file, n->n_line, n->n_col,
         trc_indent);
      if (impl->oper_typ == 'K')
         fprintf(trcfile, "&%s", impl->name);
      else
         fprintf(trcfile, "%s(", impl->name);
      }
#endif					/* TypTrc */
   /*
    * Set up the "symbol table" of dereferenced and undereferenced
    *  argument types as needed by the operation.
    */
   nparms = impl->nargs;
   j = 0;
   for (i = 0; i < num_args && i < nparms; ++i) {
      if (impl->arg_flgs[i] & RtParm) {
         CpyTyp(n_intrtyp, arg_typs->types[i], symtyps->types[j]);

#ifdef TypTrc
         if (trcfile != NULL) {
            if (i > 0)
               fprintf(trcfile, ", ");
            prt_typ(trcfile, arg_typs->types[i]);
            }
#endif					/* TypTrc */

         ++j;
         }
      if (impl->arg_flgs[i] & DrfPrm) {
         typ_deref(arg_typs->types[i], symtyps->types[j], 0);

#ifdef TypTrc
         if (trcfile != NULL) {
            if (impl->arg_flgs[i] & RtParm)
               fprintf(trcfile, "->");
            else if (i > 0)
               fprintf(trcfile, ", ");
            prt_d_typ(trcfile, arg_typs->types[i]);
            }
#endif					/* TypTrc */

         ++j;
         }
      }
   if (nparms > 0) {
      /*
       * Check for varargs. Merge remaining arguments into the
       *  type of the variable part of the parameter list.
       */
      flag = impl->arg_flgs[nparms - 1];
      if (flag & VarPrm) {
         n_vararg = num_args - nparms + 1;
         if (n_vararg < 0)
            n_vararg = 0;
         typ = symtyps->types[j - 1];
         while (i < num_args) {
            if (flag & RtParm) {
               MrgTyp(n_intrtyp, arg_typs->types[i], typ)

#ifdef TypTrc
               if (trcfile != NULL) {
                  if (i > 0)
                     fprintf(trcfile, ", ");
                  prt_typ(trcfile, arg_typs->types[i]);
                  }
#endif					/* TypTrc */

              }
            else {
               typ_deref(arg_typs->types[i], typ, 0);

#ifdef TypTrc
               if (trcfile != NULL) {
                  if (i > 0)
                     fprintf(trcfile, ", ");
                  prt_d_typ(trcfile, arg_typs->types[i]);
                  }
#endif					/* TypTrc */

              }
            ++i;
            }
         nparms -= 1; /* Don't extend with nulls into variable part */
         }
      }
   while (i < nparms) {
      if (impl->arg_flgs[i] & RtParm)
         set_typ(symtyps->types[j++], null_bit); /* Extend args with nulls */
      if (impl->arg_flgs[i] & DrfPrm)
         set_typ(symtyps->types[j++], null_bit); /* Extend args with nulls */
      ++i;
      }

   /*
    * If this operation can suspend, there may be backtracking paths
    *  to this invocation. Merge type information from those paths
    *  into the current store.
    */
   if (impl->ret_flag & DoesSusp)
      mrg_store(n->store, succ_store);

   cur_symtyps = symtyps;
   cur_rslt.bits = rslt_typ;
   cur_rslt.size = n_intrtyp;
   cur_new = n->new_types;
   infer_il(impl->in_line); /* perform inference on operation */

   if (MightFail(impl->ret_flag))
      mrg_store(succ_store, fail_store);

#ifdef TypTrc
   if (trcfile != NULL) {
      if (impl->oper_typ != 'K')
         fprintf(trcfile, ")");
      fprintf(trcfile, "  =>>  ");
      prt_typ(trcfile, rslt_typ);
      fprintf(trcfile, "\n");
      }
#endif					/* TypTrc */
   }

/*
 * chk_succ - check to see if the operation can succeed. In particular,
 *   see if it can suspend. Change the succ_store and failure store
 *   appropriately.
 */
static void chk_succ(ret_flag, susp_stor)
int ret_flag;
struct store *susp_stor;
   {
   if (ret_flag & DoesSusp) {
       if (susp_stor != NULL && (ret_flag & DoesRet))
          mrg_store(susp_stor, fail_store); /* "pass along" failure */
       fail_store = susp_stor;
       }
   else if (!(ret_flag & DoesRet)) {
      free_store(succ_store);
      succ_store = get_store(1);
      fail_store = dummy_stor;    /* shouldn't be used */
      }
   }

/*
 * infer_il - perform type inference on a piece of code within built-in
 *   operation and determine whether execution can get past it.
 */
static int infer_il(il)
struct il_code *il;
   {
   struct il_code *il1;
   int condition;
   int case_fnd;
   int ncases;
   int may_fallthru;
   int indx;
   int i;

   if (il == NULL)
      return 1;

   switch (il->il_type) {
      case IL_Const:  /* should have been replaced by literal node */
         return 0;

      case IL_If1:
         condition = eval_cond(il->u[0].fld);
         may_fallthru = (condition & MaybeFalse);
         if (condition & MaybeTrue)
            may_fallthru |= infer_il(il->u[1].fld);
         return may_fallthru;

      case IL_If2:
         condition = eval_cond(il->u[0].fld);
         may_fallthru = 0;
         if (condition & MaybeTrue)
            may_fallthru |= infer_il(il->u[1].fld);
         if (condition & MaybeFalse)
            may_fallthru |= infer_il(il->u[2].fld);
         return may_fallthru;

      case IL_Tcase1:
         type_case(il, infer_il, NULL);
         return 1;      /* no point in trying very hard here */

      case IL_Tcase2:
         indx = type_case(il, infer_il, NULL);
         if (indx != -1)
            infer_il(il->u[indx].fld);         /* default */
         return 1;      /* no point in trying very hard here */

      case IL_Lcase:
         ncases = il->u[0].n;
         indx = 1;
         case_fnd = 0;
         for (i = 0; i < ncases && !case_fnd; ++i) {
            if (il->u[indx++].n == n_vararg) {   /* selection number */
               infer_il(il->u[indx].fld);        /* action */
               case_fnd = 1;
               }
            ++indx;
            }
         if (!case_fnd)
            infer_il(il->u[indx].fld);        /* default */
         return 1;      /* no point in trying very hard here */

      case IL_Acase: {
         int maybe_int;
         int maybe_dbl;

         eval_arith((int)il->u[0].fld->u[0].n, (int)il->u[1].fld->u[0].n,
             &maybe_int, &maybe_dbl);
         if (maybe_int) {
            infer_il(il->u[2].fld);          /* C_integer action */
            if (largeints) 
               infer_il(il->u[3].fld);       /* integer action */
            }
         if (maybe_dbl)
            infer_il(il->u[4].fld);          /* C_double action */
         return 1;      /* no point in trying very hard here */
         }

      case IL_Err1:
      case IL_Err2:
         return 0;

      case IL_Block:
         return il->u[0].n;

      case IL_Call:
         return ((il->u[3].n & DoesFThru) != 0);

      case IL_Lst:
         if (infer_il(il->u[0].fld))
             return infer_il(il->u[1].fld);
         else
             return 0;

      case IL_Abstr:
         /*
          * Handle side effects.
          */
         il1 = il->u[0].fld;
         if (il1 != NULL) {
             while (il1->il_type == IL_Lst) {
                side_effect(il1->u[1].fld);
                il1 = il1->u[0].fld;
                }
             side_effect(il1);
             }

         /*
          * Set return type.
          */
         abstr_typ(il->u[1].fld, &cur_rslt);
         return 1;

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(EXIT_FAILURE);
         /* NOTREACHED */
      }
   }

/*
 * side_effect - perform a side effect from an abstract clause of a
 *  built-in operation.
 */
static void side_effect(il)
struct il_code *il;
   {
   struct type *var_typ;
   struct type *val_typ;
   struct store *store;
   int num_bits;
   int frst_bit;
   int i, j;

   /*
    * il is IL_TpAsgn, get the variable type and value type, and perform
    *  the side effect.
    */
   var_typ = get_wktyp();
   val_typ = get_wktyp();
   abstr_typ(il->u[0].fld, var_typ);    /* variable type */
   abstr_typ(il->u[1].fld, val_typ);    /* value type */

   /*
    * Determine which types that can be assigned to are in the variable
    *  type.
    *
    * Aggregate compontents.
    */
   for (i = 0; i < num_cmpnts; ++i) {
      frst_bit = compnt_array[i].frst_bit;
      num_bits = compnt_array[i].num_bits;
      store = compnt_array[i].store;
      for (j = 0; j < num_bits; ++j) {
         if (bitset(var_typ->bits, frst_bit + j))
            ChkMrgTyp(n_icntyp, val_typ->bits, store->types[j])
         }
      }

   /*
    * record fields
    */
   for (i = 0; i < n_fld; ++i)
      if (bitset(var_typ->bits, frst_fld + i))
         ChkMrgTyp(n_icntyp, val_typ->bits, fld_stor->types[i]);

   /*
    * global variables
    */
   for (i = 0; i < n_gbl; ++i)
      if (bitset(var_typ->bits, frst_gbl + i))
          MrgTyp(n_icntyp, val_typ->bits, succ_store->types[i]);

   /*
    * local variables
    */
   for (i = 0; i < n_loc; ++i)
      if (bitset(var_typ->bits, frst_loc + i))
          MrgTyp(n_icntyp, val_typ->bits, succ_store->types[n_gbl + i]);


   free_wktyp(var_typ);
   free_wktyp(val_typ);
   }

/*
 * abstr_typ - compute the type bits corresponding to an abstract type
 *  from an abstract clause of a built-in operation.
 */
static void abstr_typ(il, typ)
struct il_code *il;
struct type *typ;
   {
   struct type *typ1;
   struct type *typ2;
   struct rentry *rec;
   struct store *store;
   struct compnt_info *compnts;
   int num_bits;
   int frst_bit;
   int frst_cmpnt;
   int num_comps;
   int typcd;
   int new_indx;
   int i;
   int j;
   int indx;
   int size;
   int t_indx;
#ifdef OptimizeType
   struct typinfo *prmtyp;
#else					/* OptimizeType */
   unsigned int *prmtyp;
#endif					/* OptimizeType */

   if (il == NULL)
       return;

   switch (il->il_type) {
      case IL_VarTyp:
         /*
          * type(<parameter>)
          */
         indx = il->u[0].fld->u[0].n; /* symbol table index of variable */
         if (indx >= cur_symtyps->nsyms) {
            prmtyp = any_typ;
            size = n_rttyp;
            }
         else {
            prmtyp = cur_symtyps->types[indx];
            size = n_intrtyp;
            }
         if (typ->size < size)
            size = typ->size;
         MrgTyp(size, prmtyp, typ->bits);
         break;

      case IL_Store:
         /*
          * store[<type>]
          */
         typ1 = get_wktyp();
         abstr_typ(il->u[0].fld, typ1); /* type to be "dereferenced" */

         /*
          * Dereference types that are Icon varaibles.
          */
         typ_deref(typ1->bits, typ->bits, 0);

         /*
          * "Dereference" aggregate compontents that are not Icon variables.
          */
         for (i = 0; i < num_cmpnts; ++i) {
            if (!typecompnt[i].var) {
               if (i == stv_typ) {
                  /*
                   * Substring trapped variable stores contain variable
                   *  references, so the types are larger, but we cannot
                   *  copy more than the destination holds.
                   */
                  size = n_intrtyp;
                  if (typ->size < size)
                    size = typ->size;
                  }
               else
                  size = n_icntyp;
               frst_bit = compnt_array[i].frst_bit;
               num_bits = compnt_array[i].num_bits;
               store = compnt_array[i].store;
               for (j = 0; j < num_bits; ++j) {
                  if (bitset(typ1->bits, frst_bit + j))
                     MrgTyp(size, store->types[j], typ->bits);
                  }
               }
            }

         free_wktyp(typ1);
         break;

      case IL_Compnt:
         /*
          * <type>.<component>
          */
         typ1 = get_wktyp();
         abstr_typ(il->u[0].fld, typ1); /* type */
         i = il->u[1].n;
         if (i == CM_Fields) {
            /*
             * The all_fields component must be handled differently
             *  from the others.
             */
            frst_bit = type_array[rec_typ].frst_bit;
            num_bits = type_array[rec_typ].num_bits;
            for (i = 0; i < num_bits; ++i)
               if (bitset(typ1->bits, frst_bit + i)) {
                  rec = rec_map[i];
                  for (j = 0; j < rec->nfields; ++j)
                     set_typ(typ->bits, frst_fld + rec->frst_fld + j);
                  }
            }
         else {
            /*
             * Use component information arrays to transform type bits to
             *  the corresponding component bits.
             */
            frst_bit = type_array[typecompnt[i].aggregate].frst_bit;
            num_bits = type_array[typecompnt[i].aggregate].num_bits;
            frst_cmpnt = compnt_array[i].frst_bit;
            if (!typecompnt[i].var && typ->size < n_rttyp)
               break;   /* bad abstract type computation */ 
            for (i = 0; i < num_bits; ++i)
               if (bitset(typ1->bits, frst_bit + i))
                  set_typ(typ->bits, frst_cmpnt + i);
            free_wktyp(typ1);
            }
         break;

      case IL_Union:
         /*
          * <type 1> ++ <type 2>
          */
         abstr_typ(il->u[0].fld, typ);
         abstr_typ(il->u[1].fld, typ);
         break;

      case IL_Inter:
         /*
          * <type 1> ** <type 2>
          */
         typ1 = get_wktyp();
         typ2 = get_wktyp();
         abstr_typ(il->u[0].fld, typ1);
         abstr_typ(il->u[1].fld, typ2);
         size = n_rttyp;
#ifdef OptimizeType
         and_bits_to_packed(typ2->bits, typ1->bits, size);
#else					/* OptimizeType */
         for (i = 0; i < NumInts(size); ++i)
            typ1->bits[i] &= typ2->bits[i];
#endif					/* OptimizeType */
         if (typ->size < size)
            size = typ->size;
         MrgTyp(size, typ1->bits, typ->bits);
         free_wktyp(typ1);
         free_wktyp(typ2);
         break;

      case IL_New:
         /*
          * new <type-name>(<type 1> , ...)
          *
          * If a type was not allocated for this node, use the default
          *   one.
          */
         typ1 = get_wktyp();
         typcd = il->u[0].n;      /* type code */
         new_indx = type_array[typcd].new_indx;
         t_indx = 0;                     /* default is first index of type */
         if (cur_new != NULL && cur_new[new_indx] > 0)
            t_indx = cur_new[new_indx];

         /*
          * This RTL expression evaluates to the "new" sub-type.
          */
         set_typ(typ->bits, type_array[typcd].frst_bit + t_indx);

         /*
          * Update stores for components based on argument types in the
          *  "new" expression.
          */
         num_comps = icontypes[typcd].num_comps;
         j = icontypes[typcd].compnts;
         compnts = &compnt_array[j];
         if (typcd == stv_typ) {
            size = n_intrtyp;
            }
         else
            size = n_icntyp;
         for (i = 0; i < num_comps; ++i) {
            ClrTyp(n_rttyp, typ1->bits);
            abstr_typ(il->u[2 + i].fld, typ1);
            ChkMrgTyp(size, typ1->bits, compnts[i].store->types[t_indx]);
            }

         free_wktyp(typ1);
         break;

      case IL_IcnTyp:
         typcd_bits((int)il->u[0].n, typ);      /* type code */
         break;
      }
   }

/*
 * eval_cond - evaluate the condition of in 'if' statement from a
 *  built-in operation. The result can be both true and false because
 *  of uncertainty and because more than one execution path may be
 *  involved.
 */
static int eval_cond(il)
struct il_code *il;
   {
   int cond1;
   int cond2;

   switch (il->il_type) {
      case IL_Bang:
         cond1 = eval_cond(il->u[0].fld);
         cond2 = 0;
         if (cond1 & MaybeTrue)
            cond2 = MaybeFalse;
         if (cond1 & MaybeFalse)
            cond2 |= MaybeTrue;
         return cond2;

      case IL_And:
         cond1 = eval_cond(il->u[0].fld);
         cond2 = eval_cond(il->u[1].fld);
         return (cond1 & cond2 & MaybeTrue) | ((cond1 | cond2) & MaybeFalse);

      case IL_Cnv1:
      case IL_Cnv2:
         return eval_cnv((int)il->u[0].n, (int)il->u[1].fld->u[0].n,
            0, NULL);

      case IL_Def1:
      case IL_Def2:
         return eval_cnv((int)il->u[0].n, (int)il->u[1].fld->u[0].n,
            1, NULL);

      case IL_Is:
         return eval_is((int)il->u[0].n, il->u[1].fld->u[0].n);

      default:
         fprintf(stderr, "compiler error: unknown info in data base\n");
         exit(EXIT_FAILURE);
         /* NOTREACHED */
      }
   }

/*
 * eval_cnv - evaluate the conversion of a variable to a specific type
 *  to see if it may succeed or fail.
 */
int eval_cnv(typcd, indx, def, cnv_flags)
int typcd;       /* type to convert to */
int indx;        /* index into symbol table of variable */
int def;         /* flag: conversion has a default value */
int *cnv_flags;  /* return flag for detailed conversion information */
   {
   struct type *may_succeed;  /* types where conversion sometimes succeed */
   struct type *must_succeed; /* types where conversion always succeeds */
   struct type *must_cnv;     /* types where actual conversion is performed */
   struct type *as_is;        /* types where value already has correct type */
#ifdef OptimizeType
   struct typinfo *typ;         /* possible types of the variable */
#else					/* OptimizeType */
   unsigned int *typ;
#endif					/* OptimizeType */
   int cond;
   int i;
#ifdef OptimizeType
   unsigned int val1, val2;
#endif					/* OptimizeType */

   /*
    * Conversions may succeed for strings, integers, csets, and reals.
    *  Conversions may fail for any other types. In addition,
    *  conversions to integer or real may fail for specific values.
    */
   if (indx >= cur_symtyps->nsyms)
      return MaybeTrue | MaybeFalse;
   typ = cur_symtyps->types[indx];

   may_succeed = get_wktyp();
   must_succeed = get_wktyp();
   must_cnv = get_wktyp();
   as_is = get_wktyp();

   if (typcd == cset_typ || typcd == TypTCset) {
      set_typ(as_is->bits, cset_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, int_bit);
      set_typ(must_cnv->bits, real_bit);

      set_typ(must_succeed->bits, str_bit);
      set_typ(must_succeed->bits, cset_bit);
      set_typ(must_succeed->bits, int_bit);
      set_typ(must_succeed->bits, real_bit);
      }
   else if (typcd == str_typ || typcd == TypTStr) {
      set_typ(as_is->bits, str_bit);

      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, int_bit);
      set_typ(must_cnv->bits, real_bit);

      set_typ(must_succeed->bits, str_bit);
      set_typ(must_succeed->bits, cset_bit);
      set_typ(must_succeed->bits, int_bit);
      set_typ(must_succeed->bits, real_bit);
      }
   else if (typcd == TypCStr) {
      /*
       * as_is is empty.
       */

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, int_bit);
      set_typ(must_cnv->bits, real_bit);

      set_typ(must_succeed->bits, str_bit);
      set_typ(must_succeed->bits, cset_bit);
      set_typ(must_succeed->bits, int_bit);
      set_typ(must_succeed->bits, real_bit);
      }
   else if (typcd == real_typ) {
      set_typ(as_is->bits, real_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, int_bit);

      set_typ(must_succeed->bits, int_bit);
      set_typ(must_succeed->bits, real_bit);
      }
   else if (typcd == TypCDbl) {
      /*
       * as_is is empty.
       */

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, int_bit);
      set_typ(must_cnv->bits, real_bit);

      set_typ(must_succeed->bits, int_bit);
      set_typ(must_succeed->bits, real_bit);
      }
   else if (typcd == int_typ) {
      set_typ(as_is->bits, int_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, real_bit);

      set_typ(must_succeed->bits, int_bit);
      }
   else if (typcd == TypCInt) {
      /*
       * Note that conversion from an integer to a C integer can be
       *  done by changing the way the descriptor is accessed. It
       *  is not considered a real conversion. Conversion may fail
       *  even for integers if large integers are supported.
       */
      set_typ(as_is->bits, int_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);
      set_typ(must_cnv->bits, real_bit);

      if (!largeints)
         set_typ(must_succeed->bits, int_bit);
      }
   else if (typcd == TypEInt) {
      set_typ(as_is->bits, int_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);

      set_typ(must_succeed->bits, int_bit);
      }
   else if (typcd == TypECInt) {
      set_typ(as_is->bits, int_bit);

      set_typ(must_cnv->bits, str_bit);
      set_typ(must_cnv->bits, cset_bit);

      if (!largeints)
         set_typ(must_succeed->bits, int_bit);
      }

   MrgTyp(n_icntyp, as_is->bits, may_succeed->bits);
   MrgTyp(n_icntyp, must_cnv->bits, may_succeed->bits);
   if (def) {
      set_typ(may_succeed->bits, null_bit);
      set_typ(must_succeed->bits, null_bit);
      }

   /*
    * Determine if the conversion expression may evaluate to true or false.
    */
   cond = 0;

/*
   if (typ->bits == NULL) {
      typ->bits = alloc_mem_typ(typ->size);
      xfer_packed_types(typ);
   }
   if (may_succeed->bits->bits == NULL) {
      may_succeed->bits->bits = alloc_mem_typ(may_succeed->bits->size);
      xfer_packed_types(may_succeed->bits);
   }
   if (must_succeed->bits->bits == NULL) {
      must_succeed->bits->bits = alloc_mem_typ(must_succeed->bits->size);
      xfer_packed_types(must_succeed->bits);
   }
*/
   for (i = 0; i < NumInts(n_intrtyp); ++i) {
#ifdef OptimizeType
      if ((typ->bits != NULL) && (may_succeed->bits->bits != NULL))  {
         if (typ->bits[i] & may_succeed->bits->bits[i]) 
            cond = MaybeTrue;
      }
      else if ((typ->bits == NULL) && (may_succeed->bits->bits != NULL)) {
         val1 = get_bit_vector(typ, i);
         if (val1 & may_succeed->bits->bits[i])
            cond = MaybeTrue;
      }
      else if ((typ->bits != NULL) && (may_succeed->bits->bits == NULL)) {
         val2 = get_bit_vector(may_succeed->bits, i);
         if (typ->bits[i] & val2)
            cond = MaybeTrue;
      }
      else {
         val1 = get_bit_vector(typ, i);
         val2 = get_bit_vector(may_succeed->bits, i);
         if (val1 & val2)
            cond = MaybeTrue;
      }
      if ((typ->bits != NULL) && (must_succeed->bits->bits != NULL))  {
         if (typ->bits[i] & ~must_succeed->bits->bits[i]) 
            cond |= MaybeFalse;
      }
      else if ((typ->bits == NULL) && (must_succeed->bits->bits != NULL)) {
         val1 = get_bit_vector(typ, i);
         if (val1 & ~must_succeed->bits->bits[i])
            cond |= MaybeFalse;
      }
      else if ((typ->bits != NULL) && (must_succeed->bits->bits == NULL)) {
         val2 = get_bit_vector(must_succeed->bits, i);
         if (typ->bits[i] & ~val2) 
            cond |= MaybeFalse;       
      }
      else { 
         val1 = get_bit_vector(typ, i);
         val2 = get_bit_vector(must_succeed->bits, i);
         if (val1 & ~val2) 
            cond |= MaybeFalse;       
      }
#else					/* OptimizeType */
      if (typ[i] & may_succeed->bits[i])
         cond = MaybeTrue;
      if (typ[i] & ~must_succeed->bits[i])
         cond |= MaybeFalse;
#endif					/* OptimizeType */
   }

   /*
    * See if more detailed information about the conversion is needed.
    */
   if (cnv_flags != NULL) {
      *cnv_flags = 0;
/*
      if (as_is->bits == NULL) {
         as_is->bits->bits = alloc_mem_typ(as_is->bits->size);
         xfer_packed_types(as_is->bits);
      }
      if (must_cnv->bits->bits == NULL) {
         must_cnv->bits->bits = alloc_mem_typ(must_cnv->bits->size);
         xfer_packed_types(must_cnv->bits);
      }
*/
      for (i = 0; i < NumInts(n_intrtyp); ++i) {
#ifdef OptimizeType
         if ((typ->bits != NULL) && (as_is->bits->bits != NULL)) {
            if (typ->bits[i] & as_is->bits->bits[i]) 
               *cnv_flags |= MayKeep;
         }
         else if ((typ->bits == NULL) && (as_is->bits->bits != NULL)) {
            val1 = get_bit_vector(typ, i);
            if (val1 & as_is->bits->bits[i])
               *cnv_flags |= MayKeep;
         }
         else if ((typ->bits != NULL) && (as_is->bits->bits == NULL)) {
            val2 = get_bit_vector(as_is->bits, i);
            if (typ->bits[i] & val2)
               *cnv_flags |= MayKeep;
         }
         else  {
            val1 = get_bit_vector(typ, i);
            val2 = get_bit_vector(as_is->bits, i);
            if (val1 & val2)
               *cnv_flags |= MayKeep;
         }
         if ((typ->bits != NULL) && (must_cnv->bits->bits != NULL))  {
            if (typ->bits[i] & must_cnv->bits->bits[i]) 
               *cnv_flags |= MayConvert;
         }
         else if ((typ->bits == NULL) && (must_cnv->bits->bits != NULL)) {
            val1 = get_bit_vector(typ, i);
            if (val1 & must_cnv->bits->bits[i])
               *cnv_flags |= MayConvert;
         }
         else if ((typ->bits != NULL) && (must_cnv->bits->bits == NULL)) {
            val2 = get_bit_vector(must_cnv->bits, i);
            if (typ->bits[i] & val2)
               *cnv_flags |= MayConvert;
         }
         else  {
            val1 = get_bit_vector(typ, i);
            val2 = get_bit_vector(must_cnv->bits, i);
            if (val1 & val2)
               *cnv_flags |= MayConvert;
         }
#else					/* OptimizeType */
         if (typ[i] & as_is->bits[i])
            *cnv_flags |= MayKeep;
         if (typ[i] & must_cnv->bits[i])
            *cnv_flags |= MayConvert;
#endif					/* OptimizeType */
         }
      if (def && bitset(typ, null_bit))
         *cnv_flags |= MayDefault;
      }

   free_wktyp(may_succeed);
   free_wktyp(must_succeed);
   free_wktyp(must_cnv);
   free_wktyp(as_is);

   return cond;
   }

/*
 * eval_is - evaluate the result of an 'is' expression within a built-in
 *  operation.
 */
int eval_is(typcd, indx)
int typcd;
int indx;
   {
   int cond;
#ifdef OptimizeType
   struct typinfo *typ;
#else					/* OptimizeType */
   unsigned int *typ;
#endif					/* OptimizeType */

   if (indx >= cur_symtyps->nsyms)
      return MaybeTrue | MaybeFalse;
   typ = cur_symtyps->types[indx];
   if (has_type(typ, typcd, 0))
      cond = MaybeTrue;
   else
      cond = 0;
   if (other_type(typ, typcd))
      cond |= MaybeFalse;
   return cond;
   }

/*
 * eval_arith - determine which cases of an arith_case may be taken based
 *   on the types of its arguments.
 */
void eval_arith(indx1, indx2, maybe_int, maybe_dbl)
int indx1;
int indx2;
int *maybe_int;
int *maybe_dbl;
   {
#ifdef OptimizeType
   struct typinfo *typ1;         /* possible types of first variable */
   struct typinfo *typ2;         /* possible types of second variable */
#else					/* OptimizeType */
   unsigned int *typ1;         /* possible types of first variable */
   unsigned int *typ2;         /* possible types of second variable */
#endif					/* OptimizeType */
   int int1 = 0;
   int int2 = 0;
   int dbl1 = 0;
   int dbl2 = 0;

   typ1 = cur_symtyps->types[indx1];
   typ2 = cur_symtyps->types[indx2];

   /*
    * First see what might result if you do a convert to numeric on each
    *  variable.
    */
   if (bitset(typ1, int_bit))
      int1 = 1;
   if (bitset(typ1, real_bit))
      dbl1 = 1;
   if (bitset(typ1, str_bit) || bitset(typ1, cset_bit)) {
      int1 = 1;
      dbl1 = 1;
      }
   if (bitset(typ2, int_bit))
      int2 = 1;
   if (bitset(typ2, real_bit))
      dbl2 = 1;
   if (bitset(typ2, str_bit) || bitset(typ2, cset_bit)) {
      int2 = 1;
      dbl2 = 1;
      }

   /*
    * Use the conversion information to figure out what type of arithmetic
    *  might be done.
    */
   if (int1 && int2)
      *maybe_int = 1;
   else
      *maybe_int = 0;

   *maybe_dbl = 0;
   if (dbl1 && dbl2)
      *maybe_dbl = 1;
   else if (dbl1 && int2)
      *maybe_dbl = 1;
   else if (int1 && dbl2)
      *maybe_dbl = 1;
   }

/*
 * type_case - Determine which cases are selected in a type_case
 *  statement. This routine is used by both type inference and
 *  the code generator: a different fnc is passed in each case.
 *  In addition, the code generator passes a case_anlz structure.
 */
int type_case(il, fnc, case_anlz)
struct il_code *il;
int (*fnc)();
struct case_anlz *case_anlz;
   {
   int *typ_vect;
   int i, j;
   int num_cases;
   int num_types;
   int indx;
   int sym_indx;
   int typcd;
   int use_dflt;
#ifdef OptimizeType
   struct typinfo *typ;
#else					/* OptimizeType */
   unsigned int *typ;
#endif					/* OptimizeType */
   int select;
   struct type *wktyp;

   /*
    * Make a copy of the type of the variable the type case is
    *  working on.
    */
   sym_indx = il->u[0].fld->u[0].n; /* symbol table index */
   if (sym_indx >= cur_symtyps->nsyms)
     typ = any_typ;  /* variable is not a parameter, don't know type */
   else
     typ = cur_symtyps->types[sym_indx];
   wktyp = get_wktyp();
   CpyTyp(n_intrtyp, typ, wktyp->bits);
   typ = wktyp->bits;

   /*
    * Loop through all the case clauses.
    */
   num_cases = il->u[1].n;
   indx = 2;
   for (i = 0; i < num_cases; ++i) {
      /*
       * For each of the types selected by this clause, see if the variable's
       *  type bit vector contains that type and delete the type from the
       *  bit vector (so we know if we need the default when we are done).
       */
      num_types = il->u[indx++].n;
      typ_vect = il->u[indx++].vect;
      select = 0;
      for (j = 0; j < num_types; ++j)
         if (has_type(typ, typ_vect[j], 1)) {
            typcd = typ_vect[j];
            select += 1;
            }

      if (select > 0) {
         fnc(il->u[indx].fld);       /* action */

         /*
          * If this routine was called by the code generator, we need to
          *  return extra information.
          */
         if (case_anlz != NULL) {
            ++case_anlz->n_cases;
            if (select == 1) {
               if (case_anlz->il_then == NULL) {
                  case_anlz->typcd = typcd;
                  case_anlz->il_then = il->u[indx].fld;
                  }
               else if (case_anlz->il_else == NULL)
                  case_anlz->il_else = il->u[indx].fld;
               }
            else {
               /*
                * There is more than one possible type that will cause
                *  us to select this case. It can only be used in the "else".
                */
               if (case_anlz->il_else == NULL)
                  case_anlz->il_else = il->u[indx].fld;
               else
                  case_anlz->n_cases = 3; /* force no inlining. */
               }
            }
         }
      ++indx;
      }

   /*
    * If there are types that have not been handled, indicate this by
    *  returning the index of the default clause.
    */
   use_dflt = 0;
   for (i = 0; i < n_intrtyp; ++i)
      if (bitset(typ, i)) {
         use_dflt = 1;
         break;
         }
   free_wktyp(wktyp);
   if (use_dflt)
      return indx;
   else
      return -1;
   }

/*
 * gen_inv - general invocation. The argument list is set up, perform
 *  abstract interpretation on each possible things being invoked.
 */
static void gen_inv(typ, n)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
nodeptr n;
   {
   int ret_flag = 0;
   struct store *s_store;
   struct store *store;
   struct gentry *gptr;
   struct implement *ip;
   struct type *prc_typ;
   int frst_prc;
   int num_prcs;
   int i;

#ifdef TypTrc
   if (trcfile != NULL) {
      fprintf(trcfile, "%s (%d,%d) {\n", n->n_file, n->n_line, n->n_col);
      trc_indent = "   ";
      }
#endif					/* TypTrc */

   frst_prc = type_array[proc_typ].frst_bit;
   num_prcs = type_array[proc_typ].num_bits;

   /*
    * Dereference the type of the thing being invoked.
    */
   prc_typ = get_wktyp();
   typ_deref(typ, prc_typ->bits, 0);

   s_store = succ_store;
   store = get_store(1);

   if (bitset(prc_typ->bits, str_bit) ||
       bitset(prc_typ->bits, cset_bit) ||
       bitset(prc_typ->bits, int_bit) ||
       bitset(prc_typ->bits, real_bit)) {
      /*
       * Assume integer invocation; any argument may be the result type.
       */

#ifdef TypTrc
      if (trcfile != NULL) {
         fprintf(trcfile, "%s (%d,%d) %s{i}(", n->n_file, n->n_line, n->n_col,
            trc_indent);
         }
#endif					/* TypTrc */

      for (i = 0; i < num_args; ++i) {
         MrgTyp(n_intrtyp, arg_typs->types[i], n->type);

#ifdef TypTrc
         if (trcfile != NULL) {
            if (i > 0)
               fprintf(trcfile, ", ");
            prt_typ(trcfile, arg_typs->types[i]);
            }
#endif					/* TypTrc */

         }

      /*
       * Integer invocation may succeed or fail.
       */
      ret_flag |= DoesRet | DoesFail;
      mrg_store(s_store, store);
      mrg_store(s_store, fail_store);

#ifdef TypTrc
      if (trcfile != NULL) {
         fprintf(trcfile, ")  =>>  ");
         prt_typ(trcfile, n->type);
         fprintf(trcfile, "\n");
         }
#endif					/* TypTrc */
      }

   if (bitset(prc_typ->bits, str_bit) ||
       bitset(prc_typ->bits, cset_bit)) {
      /*
       * Assume string invocation; add all procedure types to the thing
       *  being invoked.
       */
      for (i = 0; i < num_prcs; ++i)
         set_typ(prc_typ->bits, frst_prc + i);
      }

   if (bitset(prc_typ->bits, frst_prc)) {
      /*
       * First procedure type represents all operators that are
       *  available via string invocation. Scan the operator table
       *  looking for those that are in the string invocation table.
       *  Note, this is not particularly efficient or precise.
       */
      for (i = 0; i < IHSize; ++i)
         for (ip = ohash[i]; ip != NULL; ip = ip->blink)
            if (ip->iconc_flgs & InStrTbl) {
               succ_store = cpy_store(s_store);
               infer_impl(ip, n, n->symtyps, n->type);
               ret_flag |= ip->ret_flag;
               mrg_store(succ_store, store);
               free_store(succ_store);
               }
      }

   /*
    * Check for procedure, built-in, and record constructor types
    *  and perform type inference on invocations of them.
    */
   for (i = 1; i < num_prcs; ++i)
      if (bitset(prc_typ->bits, frst_prc + i)) {
         succ_store = cpy_store(s_store);
         gptr = proc_map[i];
         switch (gptr->flag & (F_Proc | F_Builtin | F_Record)) {
            case F_Proc:
               infer_prc(gptr->val.proc, n);
               ret_flag |= gptr->val.proc->ret_flag;
               break;
            case F_Builtin:
               infer_impl(gptr->val.builtin, n, n->symtyps, n->type);
               ret_flag |= gptr->val.builtin->ret_flag;
               break;
            case F_Record:
               infer_con(gptr->val.rec, n);
               ret_flag |= DoesRet | (err_conv ? DoesFail : 0);
               break;
            }
         mrg_store(succ_store, store);
         free_store(succ_store);
         }

   /*
    * If error conversion is supported and a non-procedure value
    *   might be invoked, assume the invocation can fail.
    */
   if (err_conv && other_type(prc_typ->bits, proc_typ))
      mrg_store(s_store, fail_store);

   free_store(s_store);
   succ_store = store;
   chk_succ(ret_flag, n->store);

   free_wktyp(prc_typ);

#ifdef TypTrc
   if (trcfile != NULL) {
      fprintf(trcfile, "%s (%d,%d) }\n", n->n_file, n->n_line, n->n_col);
      trc_indent = "";
      }
#endif					/* TypTrc */
   }

/*
 * get_wktyp - get a dynamically allocated bit vector to use as a
 *  work area for doing type computations.
 */
static struct type *get_wktyp()
   {
   struct type *typ;

   if ((typ = type_pool) == NULL) {
      typ = NewStruct(type);
      typ->size = n_rttyp;
      typ->bits = alloc_typ(n_rttyp);
      }
   else {
      type_pool = type_pool->next;
      ClrTyp(n_rttyp, typ->bits);
      }
   return typ;
   }

/*
 * free_wktyp - free a dynamically allocated type bit vector.
 */
static void free_wktyp(typ)
struct type *typ;
   {
   typ->next = type_pool;
   type_pool = typ;
   }

#ifdef TypTrc

/*
 * ChkSep - supply a separating space if this is not the first item.
 */
#define ChkSep(n) (++n > 1 ? " " : "")

/*
 * prt_typ - print a type that can include variable references.
 */
static void prt_typ(file, typ)
FILE *file;
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
   {
   struct gentry *gptr;
   struct lentry *lptr;
   char *name;
   int i, j, k;
   int n;
   int frst_bit;
   int num_bits;
   char *abrv;

   fprintf(trcfile, "{");
   n = 0;
   /*
    * Go through the types and see any sub-types are present.
    */
   for (k = 0; k < num_typs; ++k) {
      frst_bit = type_array[k].frst_bit;
      num_bits = type_array[k].num_bits;
      abrv = icontypes[k].abrv;
      if (k == proc_typ) {
         /*
          * procedures, record constructors, and built-in functions.
          */
         for (i = 0; i < num_bits; ++i)
            if (bitset(typ, frst_bit + i)) {
               if (i == 0)
                  fprintf(file, "%sops", ChkSep(n));
               else {
                  gptr = proc_map[i];
                  switch (gptr->flag & (F_Proc | F_Builtin | F_Record)) {
                     case F_Proc:
                        fprintf(file, "%s%s:%s", ChkSep(n), abrv, gptr->name);
                        break;
                     case F_Builtin:
                        fprintf(file, "%sfnc:%s", ChkSep(n), gptr->name);
                        break;
                     case F_Record:
                        fprintf(file, "%sconstr:%s", ChkSep(n), gptr->name);
                        break;
                     }
                  }
               }
         }
      else if (k == rec_typ) {
         /*
          * records - include record name.
          */
         for (i = 0; i < num_bits; ++i)
            if (bitset(typ, frst_bit + i))
               fprintf(file, "%s%s:%s", ChkSep(n), abrv, rec_map[i]->name);
         }
      else if (icontypes[k].support_new | k == coexp_typ) {
         /*
          * A type with sub-types.
          */
         for (i = 0; i < num_bits; ++i)
            if (bitset(typ, frst_bit + i))
               fprintf(file, "%s%s%d", ChkSep(n), abrv, i);
         }
      else {
         /*
          * A type with no subtypes.
          */
         if (bitset(typ, frst_bit))
            fprintf(file, "%s%s", ChkSep(n), abrv);
         }
      }

   for (k = 0; k < num_cmpnts; ++k) {
      if (typecompnt[k].var) {
         /*
          * Structure component that is a variable.
          */
         frst_bit = compnt_array[k].frst_bit;
         num_bits = compnt_array[k].num_bits;
         abrv = typecompnt[k].abrv;
         for (i = 0; i < num_bits; ++i)
            if (bitset(typ, frst_bit + i))
               fprintf(file, "%s%s%d", ChkSep(n), abrv, i);
         }
      }


   /*
    * record fields
    */
   for (i = 0; i < n_fld; ++i)
      if (bitset(typ, frst_fld + i))
         fprintf(file, "%sfld%d", ChkSep(n), i);

   /*
    * global variables
    */
   for (i = 0; i < n_nmgbl; ++i)
      if (bitset(typ, frst_gbl + i)) {
         name = NULL;
         for (j = 0; j < GHSize && name == NULL; j++)
            for (gptr = ghash[j]; gptr != NULL && name == NULL; 
               gptr = gptr->blink)
                  if (gptr->index == i)
                     name = gptr->name;
         for (lptr = cur_proc->statics; lptr != NULL && name == NULL;
            lptr = lptr->next)
               if (lptr->val.index == i)
                  name = lptr->name;
         /*
          * Static variables may be returned and dereferenced in a procedure
          *  they don't belong to.
          */
         if (name == NULL)
            name = "?static?";
         fprintf(file, "%svar:%s", ChkSep(n), name);
         }

   /*
    * local variables
    */
   for (i = 0; i < n_loc; ++i)
      if (bitset(typ, frst_loc + i)) {
         name = NULL;
         for (lptr = cur_proc->args; lptr != NULL && name == NULL;
            lptr = lptr->next)
               if (lptr->val.index == i)
                  name = lptr->name;
         for (lptr = cur_proc->dynams; lptr != NULL && name == NULL;
            lptr = lptr->next)
               if (lptr->val.index == i)
                  name = lptr->name;
         /*
          * Local variables types may appear in the wrong procedure due to
          *  substring trapped variables and the inference of impossible
          *  execution paths. Make sure we don't end up with a NULL name.
          */
         if (name == NULL)
            name = "?";
         fprintf(file, "%svar:%s", ChkSep(n), name);
         }

   fprintf(trcfile, "}");
   }

/*
 * prt_d_typ - dereference a type and print it.
 */
static void prt_d_typ(file, typ)
FILE *file;
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
{
   struct type *wktyp;

   wktyp = get_wktyp();
   typ_deref(typ, wktyp->bits, 0);
   prt_typ(file, wktyp->bits);
   free_wktyp(wktyp);
}
#endif					/* TypTrc */

/*
 * get_argtyp - get an array of pointers to type bit vectors for use
 *  in constructing an argument list. The array is large enough for the
 *  largest argument list.
 */ 
static struct argtyps *get_argtyp()
   {
   struct argtyps *argtyps;

   if ((argtyps = argtyp_pool) == NULL)
#ifdef OptimizeType
     argtyps = (struct argtyps *)alloc((unsigned int)(sizeof(struct argtyps) +
      ((max_prm - 1) * sizeof(struct typinfo *))));
#else					/* OptimizeType */
     argtyps = (struct argtyps *)alloc((unsigned int)(sizeof(struct argtyps) +
      ((max_prm - 1) * sizeof(unsigned int *))));
#endif					/* OptimizeType */
   else 
      argtyp_pool = argtyp_pool->next;
   return argtyps;
   }

/*
 * free_argtyp - free array of pointers to type bitvectors.
 */
static void free_argtyp(argtyps)
struct argtyps *argtyps;
   {
   argtyps->next = argtyp_pool;
   argtyp_pool = argtyps;
   }

/*
 * varsubtyp - examine a type and determine what kinds of variable
 *  subtypes it has and whether it has any non-variable subtypes.
 *  If the type consists of a single named variable, return its symbol
 *  table entry through the parameter "singl".
 */
int varsubtyp(typ, singl)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
struct lentry **singl;
   {
   struct store *stv_stor;
   int subtypes;
   int n_types;
   int var_indx;
   int frst_bit;
   int num_bits;
   int i, j;


   subtypes = 0;
   n_types = 0;
   var_indx = -1;

   /*
    * check for non-variables.
    */
   for (i = 0; i < n_icntyp; ++i)
      if (bitset(typ, i)) {
         subtypes |= HasVal;
         ++n_types;
         }

   /* 
    * Predefined variable types.
    */
   for (i = 0; i < num_typs; ++i) {
      if (icontypes[i].deref != DrfNone) {
         frst_bit = type_array[i].frst_bit;
         num_bits = type_array[i].num_bits;
         for (j = 0; j < num_bits; ++j) {
            if (bitset(typ, frst_bit + j)) {
               if (i == stv_typ) {
                  /*
                   * We have found substring trapped variable j, see whether it
                   *  references locals or globals.
                   */
                  if (do_typinfer) {
                     stv_stor = compnt_array[str_var].store;
                     subtypes |= varsubtyp(stv_stor->types[j], NULL);
                     }
                  else
                     subtypes |= HasLcl | HasPrm | HasGlb;
                  }
               else
                  subtypes |= HasGlb;
               ++n_types;
               }
            }
         }
      }

   /*
    * Aggregate compontents that are variables.
    */
   for (i = 0; i < num_cmpnts; ++i) {
      if (typecompnt[i].var) {
         frst_bit = compnt_array[i].frst_bit;
         num_bits = compnt_array[i].num_bits;
         for (j = 0; j < num_bits; ++j) {
            if (bitset(typ, frst_bit + j)) {
               subtypes |= HasGlb;
               ++n_types;
               }
            }
         }
      }

   /*
    * record fields
    */
   for (i = 0; i < n_fld; ++i)
      if (bitset(typ, frst_fld + i)) {
         subtypes |= HasGlb;
         ++n_types;
         }

   /*
    * global variables, including statics
    */
   for (i = 0; i < n_gbl; ++i) {
      if (bitset(typ, frst_gbl + i)) {
         subtypes |= HasGlb;
         var_indx = i;
         ++n_types;
         }
      }

   /*
    * local variables
    */
   for (i = 0; i < n_loc; ++i) {
      if (bitset(typ, frst_loc + i)) {
         if (i < Abs(cur_proc->nargs))
            subtypes |= HasPrm;
         else
            subtypes |= HasLcl;
         var_indx = n_gbl + i;
         ++n_types;
         }
      }

   if (singl != NULL) {
      /*
       *  See if the type consists of a single named variable.
       */
      if (n_types == 1 && var_indx != -1)
         *singl = cur_proc->vartypmap[var_indx];
      else
         *singl = NULL;
      }

   return subtypes;
   }

/*
 * mark_recs - go through the list of parent records for this field
 *  and mark those that are in the type. Also gather information
 *  to help generate better code.
 */
void mark_recs(fp, typ, num_offsets, offset, bad_recs)
struct fentry *fp;
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
int *num_offsets;
int *offset;
int *bad_recs;
   {
   struct par_rec *rp;
   struct type *wktyp;
   int frst_rec;
   
   *num_offsets = 0;
   *offset = -1;
   *bad_recs = 0;

   wktyp = get_wktyp();
   CpyTyp(n_icntyp, typ, wktyp->bits);

   /*
    * For each record containing this field, see if the record is
    *  in the type.
    */
   frst_rec = type_array[rec_typ].frst_bit;
   for (rp = fp->rlist; rp != NULL; rp = rp->next) {
      if (bitset(wktyp->bits, frst_rec + rp->rec->rec_num)) {
         /*
          * This record is in the type.
          */
         rp->mark = 1;
         clr_typ(wktyp->bits, frst_rec + rp->rec->rec_num);
         if (*offset != rp->offset) {
            *offset = rp->offset;
            *num_offsets += 1;
            }
        }
      }

   /*
    * Are there any records that do not contain this field?
    */
   *bad_recs = has_type(wktyp->bits, rec_typ, 0);
   free_wktyp(wktyp);
   }

/*
 * past_prms - return true if execution might continue past the parameter
 *  evaluation. If a parameter has no type, this will not happen.
 */
int past_prms(n)
nodeptr n;
   {
   struct implement *impl;
   struct symtyps *symtyps;
   int nparms;
   int nargs;
   int flag;
   int i, j;

   nargs = Val0(n);
   impl = Impl1(n);
   symtyps = n->symtyps;
   nparms = impl->nargs;

   if (symtyps == NULL)
      return 1;

   j = 0;
   for (i = 0; i < nparms; ++i) {
      flag = impl->arg_flgs[i];
      if (flag & VarPrm && i >= nargs)
          break;       /* no parameters for variable part of arg list */
      if (flag & RtParm) {
         if (is_empty(symtyps->types[j]))
            return 0;
         ++j;
         }
      if (flag & DrfPrm) {
         if (is_empty(symtyps->types[j]))
            return 0;
         ++j;
         }
      }
   return 1;
   }
