/*
 * ccode.c - routines to produce internal representation of C code.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "cglobals.h"
#include "csym.h"
#include "ccode.h"
#include "ctree.h"
#include "ctoken.h"
#include "cproto.h"

#ifdef OptimizeLit

#define NO_LIMIT       0
#define LIMITED        1
#define LIMITED_TO_INT 2
#define NO_TOUCH       3

struct lit_tbl {
   int    modified;
   int    index;
   int    safe;
   struct code    *initial;
   struct code    *end;
   struct val_loc *vloc;
   struct centry  *csym; 
   struct lit_tbl *prev;
   struct lit_tbl *next;
};
#endif					/* OptimizeLit */

/*
 * Prototypes for static functions.
 */
static struct c_fnc *alc_fnc	(void);
static struct tmplftm *alc_lftm	(int num, union field *args);
static int alc_tmp 		(int n, struct tmplftm *lifetm_ary);

#ifdef	OptimizePoll
   static int analyze_poll 	(void);
   static void remove_poll 	(void);
#endif					/* OptimizePoll */

#ifdef	OptimizeLit
   static int instr		(const	char *str, int chr);
   static void invalidate	(struct val_loc *val,struct code *end,int code);
   static void analyze_literals	(struct code *start, struct code *top, int lvl);
   static int eval_code		(struct code *cd, struct lit_tbl *cur);
   static void propagate_literals (void);
   static void free_tbl		(void);
   static struct lit_tbl *alc_tbl (void);
   static void tbl_add		(truct lit_tbl *add);
#endif					/* OptimizeLit */

static struct code *asgn_null	(struct val_loc *loc1);
static struct val_loc *bound	(struct node *n, struct val_loc *rslt,
				   int catch_fail);
static struct code *check_var	(struct val_loc *d, struct code *lbl);
static void deref_cd		(struct val_loc *src, struct val_loc *dest);
static void deref_ret		(struct val_loc *src, struct val_loc *dest,
				   int subtypes);
static void endlife		(int	kind, int indx, int old, nodeptr n);
static struct val_loc *field_ref(struct node *n, struct val_loc *rslt);
static struct val_loc *gen_act	(nodeptr n, struct val_loc *rslt);
static struct val_loc *gen_apply(struct node *n, struct val_loc *rslt);
static struct val_loc *gen_args	(struct node *n, int frst_arg, int nargs);
static struct val_loc *gen_case	(struct node *n, struct val_loc *rslt);
static struct val_loc *gen_creat(struct node *n, struct val_loc *rslt);
static struct val_loc *gen_lim	(struct node *n, struct val_loc *rslt);
static struct val_loc *gen_scan	(struct node *n, struct val_loc *rslt);
static struct val_loc *gencode	(struct node *n, struct val_loc *rslt);
static struct val_loc *genretval(struct node *n, struct node *expr,
				   struct val_loc *dest);
static struct val_loc *inv_prc	(nodeptr n, struct val_loc *rslt);
static struct val_loc *inv_op	(nodeptr n, struct val_loc *rslt);
static nodeptr max_lftm		(nodeptr	n1, nodeptr n2);
static void mk_callop		(char *oper_nm, int ret_flag,
				   struct val_loc *arg1rslt, int nargs,
				   struct val_loc *rslt, int optim);
static struct code *mk_cpyval	(struct val_loc *loc1, struct val_loc *loc2);
static struct code *new_call	(void);
static char *oper_name		(struct implement *impl);
static void restr_env	(struct val_loc *sub_sav, struct val_loc *pos_sav);
static void save_env	(struct val_loc *sub_sav, struct val_loc *pos_sav);
static void setloc		(nodeptr	n);
static struct val_loc *tmp_loc	(int n);
static struct val_loc *var_ref	(struct lentry *sym);
static struct val_loc *vararg_sz(int n);

#define FrstArg 2

/*
 * Information that must be passed between a loop and its next and break
 *   expressions.
 */
struct loop_info {
   struct code *next_lbl;       /* where to branch for a next expression */
   struct code *end_loop;       /* label at end of loop */
   struct code *on_failure;     /* where to go if the loop fails */
   struct scan_info *scan_info; /* scanning environment upon entering loop */
   struct val_loc *rslt;        /* place to put result of loop */
   struct c_fnc *succ_cont;     /* the success continuation for the loop */
   struct loop_info *prev;      /* link to info for outer loop */
   };

/*
 * The allocation status of a temporary variable can either be "in use",
 *  "not allocated", or reserved for use at a code position (indicated
 *  by a specific negative number).
 */
#define InUse 1
#define NotAlc 0

/*
 * tmplftm is used to precompute lifetime information for use in allocating
 *  temporary variables.
 */
struct tmplftm {
   int cur_status;
   nodeptr lifetime;
   };

/*
 * Places where &subject and &pos are saved during string scanning. "outer"
 *  values are saved when the scanning expression is executed. "inner"
 *  values are saved when the scanning expression suspends.
 */
struct scan_info {
   struct val_loc *outer_sub;
   struct val_loc *outer_pos;
   struct val_loc *inner_sub;
   struct val_loc *inner_pos;
   struct scan_info *next;
   };

struct scan_info scan_base = {NULL, 0, NULL, 0, NULL};
struct scan_info *nxt_scan = &scan_base;

struct val_loc ignore;		 /* no values, just something to point at */
static struct val_loc proc_rslt; /* result location for procedure */

int *tmp_status = NULL;      /* allocation status of temp descriptor vars */
int *itmp_status = NULL;     /* allocation status of temp C int vars*/
int *dtmp_status = NULL;     /* allocation status of temp C double vars */
int *sbuf_status = NULL;     /* allocation of string buffers */
int *cbuf_status = NULL;     /* allocation of cset buffers */
int num_tmp;                 /* number of temp descriptors actually used */
int num_itmp;                /* number of temp C ints actually used */
int num_dtmp;                /* number of temp C doubles actually used */
int num_sbuf;                /* number of string buffers actually used */
int num_cbuf;                /* number of cset buffers actually used */
int status_sz = 20;          /* current size of tmp_status array */
int istatus_sz = 20;         /* current size of itmp_status array */
int dstatus_sz = 20;         /* current size of dtmp_status array */
int sstatus_sz = 20;         /* current size of sbuf_status array */
int cstatus_sz = 20;         /* current size of cbuf_status array */
struct freetmp *freetmp_pool = NULL;

static char frm_prfx[PrfxSz + 1];/* prefix for procedure frame */
static char *lastfiln;	     /* last file name set in code */
static int lastline;         /* last line number set in code */

#ifdef OptimizePoll
static struct code *lastpoll;
#endif					/* OptimizePoll */

#ifdef OptimizeLit
static struct lit_tbl *tbl = NULL;
static struct lit_tbl *free_lit_tbl = NULL;
#endif					/* OptimizeLit */

static struct c_fnc *fnc_lst;	/* list of C functions implementing proc */
static struct c_fnc **flst_end; /* pointer to null pointer at end of fnc_lst */
struct c_fnc *cur_fnc;   	/* C function currently being built */
static int create_lvl = 0;      /* co-expression create level */

struct pentry *cur_proc;        /* procedure currently being translated */

struct code *on_failure;	/* place to go on failure */

static struct code *p_ret_lbl;   /* label for procedure return */
static struct code *p_fail_lbl;  /* label for procedure fail */
struct code *bound_sig;   	 /* bounding signal for current procedure */

/*
 * statically declared "signals".
 */
struct code resume;
struct code contin;
struct code fallthru;
struct code next_fail;

int lbl_seq_num = 0;  /* next label sequence number */

#ifdef OptimizeLit
static void print_tbl(struct lit_tbl *start)  {
   struct lit_tbl *ptr;
   
   for (ptr=start; ptr != NULL ;ptr=ptr->next)  {
      printf("mod (%2d) strchr (%2d) ",ptr->modified,ptr->index);
      if (ptr->csym != NULL)  {
         printf("image (%13s) ",ptr->csym->image);
      }
      if (ptr->vloc != NULL)  {
         printf("val (%6d) type (%d)",ptr->vloc->u.tmp,ptr->vloc->loc_type); 
      }
      if (ptr->end == NULL)
         printf(" END IS NULL");
      printf("\n");
   }
}


static void free_tbl()  {
/*
   struct lit_tbl *ptr, *next;
*/
   free_lit_tbl = tbl;
   tbl = NULL;
/*
   ptr = tbl;
   while (ptr != NULL)  {
      next = ptr->next;
      free(ptr);
      ptr = next;
   }
   tbl = NULL;
*/
}


static struct lit_tbl *alc_tbl()  {
   struct lit_tbl *new; 
   static int cnt=0;


   if (free_lit_tbl != NULL)  {
      new = free_lit_tbl;
      free_lit_tbl = new->next;
   }
   else
      new = (struct lit_tbl *)alloc(sizeof(struct lit_tbl));
   new->modified = NO_LIMIT;
   new->index = -1;
   new->safe = 1;
   new->initial = NULL;
   new->end  = NULL;
   new->vloc = NULL;
   new->csym = NULL;
   new->prev = NULL;
   new->next = NULL;
   return new;
}
#endif					/* OptimizeLit */

/*
 * proccode - generate code for a procedure.
 */
void proccode(proc)
struct pentry *proc;
   {
   struct c_fnc *fnc;
   struct code *cd;
   struct code *cd1;
   struct code *lbl;
   nodeptr n;
   nodeptr failer;
   int gen;
   int i;
#ifdef OptimizeLit
   struct code *procstart;
#endif					/* OptimizeLit */

   /*
    * Initialize arrays used for allocating temporary variables.
    */
   if (tmp_status == NULL)
      tmp_status = (int *)alloc((unsigned int)(status_sz * sizeof(int)));
   if (itmp_status == NULL)
      itmp_status = (int *)alloc((unsigned int)(istatus_sz * sizeof(int)));
   if (dtmp_status == NULL)
      dtmp_status = (int *)alloc((unsigned int)(dstatus_sz * sizeof(int)));
   if (sbuf_status == NULL)
      sbuf_status = (int *)alloc((unsigned int)(sstatus_sz * sizeof(int)));
   if (cbuf_status == NULL)
      cbuf_status = (int *)alloc((unsigned int)(cstatus_sz * sizeof(int)));
   for (i = 0; i < status_sz; ++i)
      tmp_status[i] = NotAlloc;
   for (i = 0; i < istatus_sz; ++i)
      itmp_status[i] = NotAlloc;
   for (i = 0; i < dstatus_sz; ++i)
      dtmp_status[i] = NotAlloc;
   for (i = 0; i < sstatus_sz; ++i)
      sbuf_status[i] = NotAlloc;
   for (i = 0; i < cstatus_sz; ++i)
      cbuf_status[i] = NotAlloc;
   num_tmp = 0;
   num_itmp = 0;
   num_dtmp = 0;
   num_sbuf = 0;
   num_cbuf = 0;

   /*
    * Initialize standard signals.
    */
   resume.cd_id = C_Resume;
   contin.cd_id = C_Continue;
   fallthru.cd_id = C_FallThru;

   /*
    * Initialize procedure result and the transcan locations.
    */
   proc_rslt.loc_type = V_PRslt;
   proc_rslt.mod_access = M_None;
   ignore.loc_type = V_Ignore;
   ignore.mod_access = M_None;

   cur_proc = proc;  /* current procedure */
   lastfiln = NULL;  /* file name */
   lastline = 0;     /* line number */

#ifdef OptimizePoll
   lastpoll = NULL;
#endif					/* OptimizePoll */

   /*
    * Procedure frame prefix is the procedure prefix.
    */
   for (i = 0; i < PrfxSz; ++i)
      frm_prfx[i] = cur_proc->prefix[i];
   frm_prfx[PrfxSz] = '\0';

   /*
    * Initialize the continuation list and allocate the outer function for
    *  this procedure.
    */
   fnc_lst = NULL;
   flst_end = &fnc_lst;
   cur_fnc = alc_fnc();

#ifdef OptimizeLit
   procstart = cur_fnc->cursor;
#endif					/* OptimizeLit */

   /*
    * If the procedure is not used anywhere don't generate code for it.
    *  This can happen when using libraries containing several procedures,
    *  but not all are needed. However, if there is a block for the
    *  procedure, we need at least a dummy function.
    */
   if (!cur_proc->reachable) {
      if (!(glookup(cur_proc->name)->flag & F_SmplInv))
         outerfnc(fnc_lst);
      return;
      }

   /*
    * Allocate labels for the code for procedure failure, procedure return,
    *  and allocate the bounding signal for this procedure (at this point
    *  signals and labels are not distinguished).
    */
   p_fail_lbl = alc_lbl("proc fail", 0);
   p_ret_lbl = alc_lbl("proc return", 0);
   bound_sig = alc_lbl("bound", 0);

   n = proc->tree;
   setloc(n);
   if (Type(Tree1(n)) != N_Empty) {
      /*
       * initial clause.
       */
      Tree1(n)->lifetime = NULL;
      liveness(Tree1(n), NULL, &failer, &gen);
      if (tfatals > 0)
         return;
      lbl = alc_lbl("end initial", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(1);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) = "!first_time";
      cd->Cond = cd1;
      cd->ThenStmt = mk_goto(lbl);
      cd_add(cd);
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) = "first_time = 0;";
      cd_add(cd);
      bound(Tree1(n), &ignore, 1);
      cur_fnc->cursor = lbl;
      }
   Tree2(n)->lifetime = NULL;
   liveness(Tree2(n), NULL, &failer, &gen);
   if (tfatals > 0)
      return;
   bound(Tree2(n), &ignore, 1);

   /*
    * Place code to perform procedure failure and return and the
    *  end of the outer function.
    */
   setloc(Tree3(n));
   cd_add(p_fail_lbl);
   cd = NewCode(0);
   cd->cd_id = C_PFail;
   cd_add(cd);
   cd_add(p_ret_lbl);
   cd = NewCode(0);
   cd->cd_id = C_PRet;
   cd_add(cd);

   /*
    * Fix up signal handling code and perform peephole optimizations.
    */
   fix_fncs(fnc_lst);

#ifdef OptimizeLit
    analyze_literals(procstart, NULL, 0);
    propagate_literals();
#endif					/* OptimizeLit */

   /*
    * The outer function is the first one on the list. It has the
    *  procedure interface; the others are just continuations.
    */
   outerfnc(fnc_lst);
   for (fnc = fnc_lst->next; fnc != NULL; fnc = fnc->next)
      if (fnc->ref_cnt > 0)
         prt_fnc(fnc);
#ifdef OptimizeLit
   free_tbl();
#endif					/* OptimizeLit */
}

/*
 * gencode - generate code for a syntax tree.
 */
static struct val_loc *gencode(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct code *cd;
   struct code *cd1;
   struct code *fail_sav;
   struct code *lbl1;
   struct code *lbl2;
   struct code *cursor_sav;
   struct c_fnc *fnc_sav;
   struct c_fnc *fnc;
   struct implement *impl;
   struct implement *impl1;
   struct val_loc *r1[3];
   struct val_loc *r2[2];
   struct val_loc *frst_arg;
   struct lentry *single;
   struct freetmp *freetmp;
   struct freetmp *ft;
   struct tmplftm *lifetm_ary;
   char *sbuf;
   int i;
   int tmp_indx;
   int nargs;
   static struct loop_info *loop_info = NULL;
   struct loop_info *li_sav;

   switch (n->n_type) {
      case N_Activat:
         rslt = gen_act(n, rslt);
         break;

      case N_Alt:
         rslt = chk_alc(rslt, n->lifetime); /* insure a result location */

         fail_sav = on_failure;
         fnc_sav = cur_fnc;

         /*
          * If the first alternative fails, execution must go to the
          *  "alt" label.
          */
         lbl1 = alc_lbl("alt", 0);
         on_failure = lbl1;

         cd_add(lbl1);
         cur_fnc->cursor = lbl1->prev;  /* 1st alternative goes before label */
         gencode(Tree0(n), rslt);

         /*
          * Each alternative must call the same success continuation.
          */
         fnc = alc_fnc();
         callc_add(fnc);

         cur_fnc = fnc_sav;             /* return to the context of the label */
         cur_fnc->cursor = lbl1;        /* 2nd alternative goes after label */
         on_failure = fail_sav;         /* on failure, alternation fails */
         gencode(Tree1(n), rslt);
         callc_add(fnc);                /* call continuation */

         /*
          * Code following the alternation goes in the continuation. If
          *  the code fails, the continuation returns the resume signal.
          */
         cur_fnc = fnc;
         on_failure = &resume;
         break;

      case N_Apply:
         rslt = gen_apply(n, rslt);
         break;

      case N_Augop:
         impl = Impl0(n);       /* assignment */
         impl1 = Impl1(n);      /* the operation */
         if (impl == NULL || impl1 == NULL) {
            rslt = &ignore;    /* make sure code generation can continue */
            break;
            }

         /*
          * allocate an argument list for the operation.
          */
         lifetm_ary = alc_lftm(2, &n->n_field[2]);
         tmp_indx = alc_tmp(2, lifetm_ary);
         r1[0] = tmp_loc(tmp_indx);
         r1[1] = tmp_loc(tmp_indx + 1);

         gencode(Tree2(n), r1[0]);  /* first argument */

         /*
          * allocate an argument list for the assignment and copy the
          *  value of the first argument into it.
          */
         lifetm_ary[0].cur_status = InUse;
         lifetm_ary[1].cur_status = n->postn;
         lifetm_ary[1].lifetime = n->intrnl_lftm;
         tmp_indx = alc_tmp(2, lifetm_ary);
         r2[0] = tmp_loc(tmp_indx++);
         cd_add(mk_cpyval(r2[0], r1[0]));
         r2[1] = tmp_loc(tmp_indx);

         gencode(Tree3(n), r1[1]); /* second argument */

         /*
          * Produce code for the operation.
          */
         setloc(n);
         implproto(impl1);
         mk_callop(oper_name(impl1), impl1->ret_flag, r1[0], 2, r2[1], 0);

         /*
          * Produce code for the assignment.
          */
         implproto(impl);
         if (impl->ret_flag & (DoesRet | DoesSusp))
            rslt = chk_alc(rslt, n->lifetime);
         mk_callop(oper_name(impl), impl->ret_flag, r2[0], 2, rslt, 0);

         free((char *)lifetm_ary);
         break;

      case N_Bar: {
         struct val_loc *fail_flg;

         /*
          * Allocate an integer variable to keep track of whether the
          *  repeated alternation should fail when execution reaches
          *  the top of its loop, and generate code to initialize the
          *  variable to 0.
          */
         fail_flg = itmp_loc(alc_itmp(n->intrnl_lftm));
         cd = alc_ary(2);
         cd->ElemTyp(0) = A_ValLoc;
         cd->ValLoc(0) =                fail_flg;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                   " = 0;";
         cd_add(cd);

         /*
          * Code at the top of the repeated alternation loop checks
          *  the failure flag.
          */
         lbl1 = alc_lbl("rep alt", 0);
         cd_add(lbl1);
         cd = NewCode(2);
         cd->cd_id = C_If;
         cd1 = alc_ary(1);
         cd1->ElemTyp(0) = A_ValLoc;
         cd1->ValLoc(0) = fail_flg;
         cd->Cond = cd1;
         cd->ThenStmt = sig_cd(on_failure, cur_fnc);
         cd_add(cd);

         /*
          * If the expression fails without producing a value, the
          *  repeated alternation must fail.
          */
         cd = alc_ary(2);
         cd->ElemTyp(0) = A_ValLoc;
         cd->ValLoc(0) =                fail_flg;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                   " = 1;";
         cd_add(cd);

         /*
          * Generate code for the repeated expression. If it produces
          *  a value before before backtracking occurs, the loop is
          *  repeated as indicated by the value of the failure flag.
          */
         on_failure = lbl1;
         rslt = gencode(Tree0(n), rslt);
         cd = alc_ary(2);
         cd->ElemTyp(0) = A_ValLoc;
         cd->ValLoc(0) =                fail_flg;
         cd->ElemTyp(1) = A_Str;
         cd->Str(1) =                   " = 0;";
         cd_add(cd);
         }
        break;

      case N_Break:
         if (loop_info == NULL) {
            nfatal(n, "invalid context for a break expression", NULL);
            rslt = &ignore;
            break;
            }

         /*
          * If the break is in a different string scanning context from the
          *  loop itself, generate code to restore the scanning environment.
          */
         if (nxt_scan != loop_info->scan_info)
            restr_env(loop_info->scan_info->outer_sub,
               loop_info->scan_info->outer_pos);


         if (Tree0(n)->n_type == N_Empty && loop_info->rslt == &ignore) {
             /*
              * The break has no associated expression and the loop needs
              *  no value, so just branch out of the loop.
              */
             cd_add(sig_cd(loop_info->end_loop, cur_fnc));
             }
         else {
            /*
             * The code for the expression associated with the break is
             *  actually placed at the end of the loop. Go there and
             *  add a label to branch to.
             */
            cursor_sav = cur_fnc->cursor;
            fnc_sav = cur_fnc;
            fail_sav = on_failure;
            cur_fnc = loop_info->end_loop->Container;
            cur_fnc->cursor = loop_info->end_loop->prev;
            on_failure = loop_info->on_failure;
            lbl1 = alc_lbl("break", 0);
            cd_add(lbl1);

            /*
             * Make sure a result location has been allocated for the
             *  loop, restore the loop information for the next outer
             *  loop, generate code for the break expression, then
             *  restore the loop information for this loop.
             */
            loop_info->rslt = chk_alc(loop_info->rslt, Tree0(n)->lifetime);
            li_sav = loop_info;
            loop_info = loop_info->prev;
            gencode(Tree0(n), li_sav->rslt);
            loop_info = li_sav;

            /*
             * If this or another break expression suspends so we cannot
             *  just branch to the end of the loop, all breaks must
             *  call a common continuation.
             */
            if (cur_fnc->cursor->next != loop_info->end_loop &&
                loop_info->succ_cont == NULL)
               loop_info->succ_cont = alc_fnc();
            if (loop_info->succ_cont == NULL)
               cd_add(mk_goto(loop_info->end_loop)); /* go to end of loop */
            else
               callc_add(loop_info->succ_cont);      /* call continuation */

            /*
             * Return to the location of the break and generate a branch to
             *  the code for its associated expression.
             */
            cur_fnc = fnc_sav;
            cur_fnc->cursor = cursor_sav;
            on_failure = fail_sav;
            cd_add(sig_cd(lbl1, cur_fnc));
            }
         rslt = &ignore;   /* shouldn't be used but must be something valid */
         break;

      case N_Case:
         rslt = gen_case(n, rslt);
         break;

      case N_Create:
         rslt = gen_creat(n, rslt);
         break;

      case N_Cset:
      case N_Int:
      case N_Real:
      case N_Str:
         cd = NewCode(2);
         cd->cd_id = C_Lit;
         rslt = chk_alc(rslt, n->lifetime);
         cd->Rslt = rslt;
         cd->Literal = CSym0(n);
         cd_add(cd);
         break;

      case N_Empty:
         /*
          * Assume null value is needed.
          */
         if (rslt == &ignore)
           break;
         rslt = chk_alc(rslt, n->lifetime);
         cd_add(asgn_null(rslt));
         break;

      case N_Field:
         rslt = field_ref(n, rslt);
         break;

      case N_Id:
         /*
          * If the variable reference is not going to be used, don't bother
          *  building it.
          */
         if (rslt == &ignore)
           break;
         cd = NewCode(2);
         cd->cd_id = C_NamedVar;
         rslt = chk_alc(rslt, n->lifetime);
         cd->Rslt = rslt;
         cd->NamedVar = LSym0(n);
         cd_add(cd);
         break;

      case N_If:
         if (Type(Tree2(n)) == N_Empty) {
            /*
             * if-then. Control clause is bounded, but otherwise trivial.
             */ 
            bound(Tree0(n), &ignore, 0);  	/* control clause */
            rslt = gencode(Tree1(n), rslt);     /* then clause */
            }
         else {
            /*
             * if-then-else. Establish an "else" label as the failure
             *   label of the bounded control clause.
             */
            fail_sav = on_failure;
            fnc_sav = cur_fnc;
            lbl1 = alc_lbl("else", 0);
            on_failure = lbl1;

            bound(Tree0(n), &ignore, 0);  /* control clause */

            cd_add(lbl1);
            cur_fnc->cursor = lbl1->prev; /* then clause goes before else lbl */
            on_failure = fail_sav;
            rslt = chk_alc(rslt, n->lifetime);
            gencode(Tree1(n), rslt);      /* then clause */

            /*
             * If the then clause is not a generator, execution can
             *  just go to the end of the if-then-else expression. If it
             *  is a generator, the continuation for the expression must be
             *  in a separate function.
             */
            if (cur_fnc->cursor->next == lbl1) {
               fnc = NULL;
               lbl2 = alc_lbl("end if", 0);
               cd_add(mk_goto(lbl2));
               cur_fnc->cursor = lbl1;
               cd_add(lbl2);
               }
            else {
               lbl2 = NULL;
               fnc = alc_fnc();
               callc_add(fnc);
               cur_fnc = fnc_sav;
               }

            cur_fnc->cursor = lbl1;    /* else clause goes after label */
            on_failure = fail_sav;
            gencode(Tree2(n), rslt);   /* else clause */

            /*
             * If the else clause is not a generator, execution is at
             *  the end of the if-then-else expression, but the if clause
             *  may have forced the continuation to be in a separate function.
             *  If the else clause is a generator, it forces the continuation
             *  to be in a separate function.
             */
            if (fnc == NULL) {
               if (cur_fnc->cursor->next == lbl2)
                  cur_fnc->cursor = lbl2;
               else {
                  fnc = alc_fnc();
                  callc_add(fnc);
                  /*
                   * The then clause is not a generator, so it has branched
                   *  to lbl2. We must add a call to the continuation there.
                   */
                  cur_fnc = fnc_sav;
                  cur_fnc->cursor = lbl2;
                  on_failure = fail_sav;
                  callc_add(fnc);
                  }
               }
            else
               callc_add(fnc);

            if (fnc != NULL) {
               /*
                * We produced a continuation for the if-then-else, so code
                *  generation must proceed in it.
                */
               cur_fnc = fnc;
               on_failure = &resume;
               }
            }
         break;

      case N_Invok:
         /*
          * General invocation.
          */
         nargs = Val0(n);
         if (Tree1(n)->n_type == N_Empty) {
            /*
             * Mutual evaluation.
             */
            for (i = 2; i <= nargs; ++i)
               gencode(n->n_field[i].n_ptr, &ignore);   /* arg i - 1 */
            rslt = chk_alc(rslt, n->lifetime);
            gencode(n->n_field[nargs + 1].n_ptr, rslt); /* last argument */
            }
         else {
            ++nargs; /* consider the procedure an argument to invoke() */
            frst_arg = gen_args(n, 1, nargs);
            setloc(n);
            /*
             * Assume this operation uses its result location as a work
             *   area. Give it a location that is tended, where the value
             *   is retained as long as the operation can be resumed.
             */
            if (rslt == &ignore)
               rslt = NULL;      /* force allocation of temporary */
            rslt = chk_alc(rslt, max_lftm(n->lifetime, n->intrnl_lftm));
            mk_callop( "invoke", DoesRet | DoesFail | DoesSusp, frst_arg, nargs,
               rslt, 0);
            }
         break;

      case N_InvOp:
         rslt = inv_op(n, rslt);
         break;

      case N_InvProc:
         rslt = inv_prc(n, rslt);
         break;

      case N_InvRec: {
         /*
          * Directly invoke a record constructor.
          */
         struct rentry *rec;

         nargs = Val0(n);             /* number of arguments */
         frst_arg = gen_args(n, 2, nargs);
         setloc(n);
         rec = Rec1(n);

         rslt = chk_alc(rslt, n->lifetime);

         /*
          * If error conversion can occur then the record constructor may
          *  fail and we must check the signal.
          */
         if (err_conv) {
            sbuf = (char *)alloc((unsigned int)(strlen(rec->name) + 
                strlen("signal = R_") + PrfxSz + 1));
            sprintf(sbuf, "signal = R%s_%s(", rec->prefix, rec->name);
            }
         else {
            sbuf = (char *)alloc((unsigned int)(strlen(rec->name) + PrfxSz +4));
            sprintf(sbuf, "R%s_%s(", rec->prefix, rec->name);
            }
         cd = alc_ary(9);
         cd->ElemTyp(0) = A_Str;        /* constructor name */
         cd->Str(0) = sbuf;
         cd->ElemTyp(1) = A_Intgr;      /* number of arguments */
         cd->Intgr(1) = nargs;
         cd->ElemTyp(2) = A_Str;        /* , */
         cd->Str(2) = ", ";
         if (frst_arg == NULL) {        /* location of first argument */
            cd->ElemTyp(3) = A_Str;
            cd->Str(3) = "NULL";
            cd->ElemTyp(4) = A_Str;
            cd->Str(4) = "";
            }
         else {
            cd->ElemTyp(3) = A_Str;
            cd->Str(3) = "&";
            cd->ElemTyp(4) = A_ValLoc;
            cd->ValLoc(4) = frst_arg;
            }
         cd->ElemTyp(5) = A_Str;        /* , */
         cd->Str(5) = ", ";
         cd->ElemTyp(6) = A_Str;        /* location of result */
         cd->Str(6) = "&";
         cd->ElemTyp(7) = A_ValLoc;
         cd->ValLoc(7) = rslt;
         cd->ElemTyp(8) = A_Str;
         cd->Str(8) =                   ");";
         cd_add(cd);
         if (err_conv) {
            cd = NewCode(2);
            cd->cd_id = C_If;
            cd1 = alc_ary(1);
            cd1->ElemTyp(0) = A_Str;
            cd1->Str(0) =                  "signal == A_Resume";
            cd->Cond = cd1;
            cd->ThenStmt = sig_cd(on_failure, cur_fnc);
            cd_add(cd);
            }
         }
         break;

      case N_Limit:
         rslt = gen_lim(n, rslt);
         break;

      case N_Loop: {
         struct loop_info li;

         /*
          * Set up loop information for use by break and next expressions.
          */
         li.end_loop = alc_lbl("end loop", 0);
         cd_add(li.end_loop);
         cur_fnc->cursor = li.end_loop->prev;      /* loop goes before label */
         li.rslt = rslt;
         li.on_failure = on_failure;
         li.scan_info = nxt_scan;
         li.succ_cont = NULL;
         li.prev = loop_info;
         loop_info = &li;

         switch ((int)Val0(Tree0(n))) {
            case EVERY:
               /*
                * "next" in the control clause just fails.
                */
               li.next_lbl = &next_fail;
               gencode(Tree1(n), &ignore);          /* control clause */
               /*
                * "next" in the do clause transfers control to the
                *   statement at the end of the loop that resumes the
                *   control clause.
                */
               li.next_lbl = alc_lbl("next", 0);
               bound(Tree2(n), &ignore, 1);         /* do clause */
               cd_add(li.next_lbl);
               cd_add(sig_cd(on_failure, cur_fnc)); /* resume control clause */
               break;

            case REPEAT:
               li.next_lbl = alc_lbl("repeat", 0);
               cd_add(li.next_lbl);
               bound(Tree1(n), &ignore, 1);
               cd_add(mk_goto(li.next_lbl));
               break;

            case SUSPEND:			/* suspension expression */
               if (create_lvl > 0) {
                  nfatal(n, "invalid context for suspend", NULL);
                  return &ignore;
                  }
               /*
                * "next" in the control clause just fails. The result
                *   of the control clause goes in the procedure return
                *   location.
                */
               li.next_lbl = &next_fail;
               genretval(n, Tree1(n), &proc_rslt);

               /*
                * If necessary, swap scanning environments before suspending.
                *   if there is no success continuation, just return.
                */
               if (nxt_scan != &scan_base) {
                  save_env(scan_base.inner_sub, scan_base.inner_pos);
                  restr_env(scan_base.outer_sub, scan_base.outer_pos);
                  }
               cd = NewCode(2);
               cd->cd_id = C_If;
               cd1 = alc_ary(2);
               cd1->ElemTyp(0) = A_ProcCont;
               cd1->ElemTyp(1) = A_Str;
               cd1->Str(1) = " == NULL";
               cd->Cond = cd1;
               cd->ThenStmt = sig_cd(p_ret_lbl, cur_fnc);
               cd_add(cd);
               cd = NewCode(0);
               cd->cd_id = C_PSusp;
               cd_add(cd);
               cur_fnc->flag |= CF_ForeignSig;

               /*
                * Force updating file name and line number, and if needed,
                *  switch scanning environments before resuming.
                */
               lastfiln = NULL;
               lastline = 0;
               if (nxt_scan != &scan_base) {
                  save_env(scan_base.outer_sub, scan_base.outer_pos);
                  restr_env(scan_base.inner_sub, scan_base.inner_pos);
                  }

               /*
                * "next" in the do clause transfers control to the
                *   statement at the end of the loop that resumes the
                *   control clause.
                */
               li.next_lbl = alc_lbl("next", 0);
               bound(Tree2(n), &ignore, 1);       /* do clause */
               cd_add(li.next_lbl);
               cd_add(sig_cd(on_failure, cur_fnc));
               break;

            case WHILE:
               li.next_lbl = alc_lbl("while", 0);
               cd_add(li.next_lbl);
               /*
                * The control clause and do clause are both bounded expressions,
                *   but only the do clause establishes a new failure label.
                */
               bound(Tree1(n), &ignore, 0);      /* control clause */
               bound(Tree2(n), &ignore, 1);      /* do clause */
               cd_add(mk_goto(li.next_lbl));
               break;

            case UNTIL:
               fail_sav = on_failure;
               li.next_lbl = alc_lbl("until", 0);
               cd_add(li.next_lbl);

               /*
                * If the control clause fails, execution continues in
                *  the loop.
                */
               if (Type(Tree2(n)) == N_Empty)
                  on_failure = li.next_lbl;  
               else {
                  lbl2 = alc_lbl("do", 0);
                  on_failure = lbl2;
                  cd_add(lbl2);
                  cur_fnc->cursor = lbl2->prev;  /* control before label */
                  }
               bound(Tree1(n), &ignore, 0);      /* control clause */

               /*
                * If the control clause succeeds, the loop fails.
                */
               cd_add(sig_cd(fail_sav, cur_fnc));

               if (Type(Tree2(n)) != N_Empty) {
                  /*
                   * Do clause goes after the label and the loop repeats.
                   */
                  cur_fnc->cursor = lbl2;
                  bound(Tree2(n), &ignore, 1);      /* do clause */
                  cd_add(mk_goto(li.next_lbl));
                  }
               break;
            }

         /*
          * Go to the end of the loop and see if the loop's success continuation
          *  is in a separate function.
          */
         cur_fnc = li.end_loop->Container;
         cur_fnc->cursor = li.end_loop;
         if (li.succ_cont != NULL) {
            callc_add(li.succ_cont);
            cur_fnc = li.succ_cont;
            on_failure = &resume;
            }
         if (li.rslt == NULL)
            rslt = &ignore; /* shouldn't be used but must be something valid */
         else
            rslt = li.rslt;
         loop_info = li.prev;
         break;
         }

      case N_Next:
         /*
          * In some contexts "next" just fails. In other contexts it
          *   transfers control to a label, in which case it may have
          *   to restore a scanning environment.
          */
         if (loop_info == NULL)
            nfatal(n, "invalid context for a next expression", NULL);
         else if (loop_info->next_lbl == &next_fail)
            cd_add(sig_cd(on_failure, cur_fnc));
         else {
            if (nxt_scan != loop_info->scan_info)
               restr_env(loop_info->scan_info->outer_sub,
                  loop_info->scan_info->outer_pos);
            cd_add(sig_cd(loop_info->next_lbl, cur_fnc));
            }
         rslt = &ignore; /* shouldn't be used but must be something valid */
         break;

      case N_Not:
         lbl1 = alc_lbl("not", 0);
         fail_sav = on_failure;
         on_failure = lbl1;
         cd_add(lbl1);
         cur_fnc->cursor = lbl1->prev;        /* code goes before label */
         bound(Tree0(n), &ignore, 0);
         on_failure = fail_sav;
         cd_add(sig_cd(on_failure, cur_fnc)); /* convert success to failure */
         cur_fnc->cursor = lbl1;	      /* convert failure to null */
         if (rslt != &ignore) {
            rslt = chk_alc(rslt, n->lifetime);
            cd_add(asgn_null(rslt));
            }
         break;

      case N_Ret:
         if (create_lvl > 0) {
            nfatal(n, "invalid context for return or fail", NULL);
            return &ignore;
            }
         if (Val0(Tree0(n)) == RETURN) {
            /*
             * Set up the failure action of the return expression to do a
             *  procedure fail.
             */
            if (nxt_scan != &scan_base) {
               /*
                * we must switch scanning environments if the expression fails.
                */
               lbl1 = alc_lbl("return fail", 0);
               cd_add(lbl1);
               restr_env(scan_base.outer_sub, scan_base.outer_pos);
               cd_add(sig_cd(p_fail_lbl, cur_fnc));
               cur_fnc->cursor = lbl1->prev;        /* code goes before label */
               on_failure = lbl1;
               }
            else
               on_failure = p_fail_lbl;

            /*
             * Produce code to place return value in procedure result location.
             */
            genretval(n, Tree1(n), &proc_rslt);

            /*
             * See if a scanning environment must be restored and
             *  transfer control to the procedure return code.
             */
            if (nxt_scan != &scan_base)
               restr_env(scan_base.outer_sub, scan_base.outer_pos);
            cd_add(sig_cd(p_ret_lbl, cur_fnc));
            }
         else {
            /*
             * fail. See if a scanning environment must be restored and
             *  transfer control to the procedure failure code.
             */
            if (nxt_scan != &scan_base)
               restr_env(scan_base.outer_sub, scan_base.outer_pos);
            cd_add(sig_cd(p_fail_lbl, cur_fnc));
            }
         rslt = &ignore; /* shouldn't be used but must be something valid */
         break;

      case N_Scan:
         rslt = gen_scan(n, rslt);
         break;

      case N_Sect:
         /*
          * x[i+:j] or x[i-:j] (x[i:j] handled as ordinary operator)
          */
         impl1 = Impl0(n);     /* sectioning */
         if (impl1 == NULL) {
            rslt = &ignore;    /* make sure code generation can continue */
            break;
            }
         implproto(impl1);

         impl = Impl1(n);      /* plus or minus */
         /*
          * Allocate work area of temporary variables for sectioning.
          */
         lifetm_ary = alc_lftm(3, NULL);
         lifetm_ary[0].cur_status = Tree2(n)->postn;
         lifetm_ary[0].lifetime = n->intrnl_lftm;
         lifetm_ary[1].cur_status = Tree3(n)->postn;
         lifetm_ary[1].lifetime = n->intrnl_lftm;
         lifetm_ary[2].cur_status = n->postn;
         lifetm_ary[2].lifetime = n->intrnl_lftm;
         tmp_indx = alc_tmp(3, lifetm_ary);
         for (i = 0; i < 3; ++i)
            r1[i] = tmp_loc(tmp_indx++);
         gencode(Tree2(n), r1[0]);   /* generate code to compute x */
         gencode(Tree3(n), r1[1]);   /* generate code compute i */

         /*
          * Allocate work area of temporary variables for arithmetic.
          */
         lifetm_ary[0].cur_status = InUse;
         lifetm_ary[0].lifetime = Tree3(n)->lifetime;
         lifetm_ary[1].cur_status = Tree4(n)->postn;
         lifetm_ary[1].lifetime = Tree4(n)->lifetime;
         tmp_indx = alc_tmp(2, lifetm_ary);
         for (i = 0; i < 2; ++i)
            r2[i] = tmp_loc(tmp_indx++);
         cd_add(mk_cpyval(r2[0], r1[1])); /* generate code to copy i */
         gencode(Tree4(n), r2[1]);        /* generate code to compute j */

         /*
          * generate code for i op j.
          */
         setloc(n);
         implproto(impl);
         mk_callop(oper_name(impl), impl->ret_flag, r2[0], 2, r1[2], 0);

         /*
          * generate code for x[i : (i op j)]
          */
         rslt = chk_alc(rslt, n->lifetime);
         mk_callop(oper_name(impl1),impl1->ret_flag,r1[0],3,rslt,0);
         free((char *)lifetm_ary);
         break;

      case N_Slist:
         bound(Tree0(n), &ignore, 1);
         rslt = gencode(Tree1(n), rslt);
         break;

      case N_SmplAsgn: {
         struct val_loc *var, *val;

         /*
          * Optimized assignment to a named variable. Use information
          *  from type inferencing to determine if the right-hand-side
          *  is a variable.
          */
         var = var_ref(LSym0(Tree2(n)));
         if (HasVar(varsubtyp(Tree3(n)->type, &single)))
            Val0(n) = AsgnDeref;
         if (single != NULL) {
            /*
             * Right-hand-side results in a named variable. Compute
             *  the expression but don't bother saving the result, we
             *  know what it is. Assignment just copies value from
             *  one variable to the other.
             */
            gencode(Tree3(n), &ignore);
            val = var_ref(single);
            cd_add(mk_cpyval(var, val));
            }
         else switch (Val0(n)) { 
            case AsgnDirect:
               /*
                * It is safe to compute the result directly into the variable.
                */
               gencode(Tree3(n), var);
               break;
            case AsgnCopy:
               /*
                * The result is not a variable reference, but it is not
                *  safe to compute it into the variable, we must use a
                *  temporary variable.
                */
               val = gencode(Tree3(n), NULL);
               cd_add(mk_cpyval(var, val));
               break;
            case AsgnDeref:
               /*
                * We must dereference the result into the variable.
                */
               val = gencode(Tree3(n), NULL);
               deref_cd(val, var);
               break;
            }

         /*
          * If the assignment has to produce a result, construct the
          *  variable reference.
          */
         if (rslt != &ignore)
            rslt = gencode(Tree2(n), rslt);
         }
         break;

      case N_SmplAug: {
         /*
          * Optimized augmented assignment to a named variable.
          */
         struct val_loc *var, *val;

         impl = Impl1(n);      /* the operation */
         if (impl == NULL) {
            rslt = &ignore;    /* make sure code generation can continue */
            break;
            }

         implproto(impl); /* insure prototype for operation */

         /*
          * Generate code to compute the arguments for the operation.
          */
         frst_arg = gen_args(n, 2, 2);
         setloc(n);

         /*
          * Use information from type inferencing to determine if the
          *  operation produces a variable.
          */
         if (HasVar(varsubtyp(Typ4(n), &single)))
            Val0(n) = AsgnDeref;
         var = var_ref(LSym0(Tree2(n)));
         if (single != NULL) {
            /*
             * The operation results in a named variable. Call the operation
             *  but don't bother saving the result, we know what it is.
             *  Assignment just copies value from one variable to the other.
             */
            mk_callop(oper_name(impl), impl->ret_flag, frst_arg, 2,
                  &ignore, 0);
            val = var_ref(single);
            cd_add(mk_cpyval(var, val));
            }
         else switch (Val0(n)) { 
            case AsgnDirect:
               /*
                * It is safe to compute the result directly into the variable.
                */
               mk_callop(oper_name(impl), impl->ret_flag, frst_arg, 2,
                  var, 0);
               break;
            case AsgnCopy:
               /*
                * The result is not a variable reference, but it is not
                *  safe to compute it into the variable, we must use a
                *  temporary variable.
                */
               val = chk_alc(NULL, n);
               mk_callop(oper_name(impl), impl->ret_flag, frst_arg, 2, val, 0);
               cd_add(mk_cpyval(var, val));
               break;
            case AsgnDeref:
               /*
                * We must dereference the result into the variable.
                */
               val = chk_alc(NULL, n);
               mk_callop(oper_name(impl), impl->ret_flag, frst_arg, 2, val, 0);
               deref_cd(val, var);
               break;
            }

         /*
          * If the assignment has to produce a result, construct the
          *  variable reference.
          */
         if (rslt != &ignore)
            rslt = gencode(Tree2(n), rslt);
         }
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }

   /*
    * Free any temporaries whose lifetime ends at this node.
    */
   freetmp = n->freetmp;
   while (freetmp != NULL) {
      switch (freetmp->kind) {
         case DescTmp:
            tmp_status[freetmp->indx] = freetmp->old;
            break;
         case CIntTmp:
            itmp_status[freetmp->indx] = freetmp->old;
            break;
         case CDblTmp:
            dtmp_status[freetmp->indx] = freetmp->old;
            break;
         case SBuf:
            sbuf_status[freetmp->indx] = freetmp->old;
            break;
         case CBuf:
            cbuf_status[freetmp->indx] = freetmp->old;
            break;
         }
      ft = freetmp->next;
      freetmp->next = freetmp_pool;
      freetmp_pool = freetmp;
      freetmp = ft;
      }
   return rslt;
   }

/*
 * chk_alc - make sure a result location has been allocated. If it is
 *  a temporary variable, indicate that it is now in use.
 */
struct val_loc *chk_alc(rslt, lifetime)
struct val_loc *rslt;
nodeptr lifetime;
   {
   struct tmplftm tmplftm;

   if (rslt == NULL) {
      if (lifetime == NULL)
         rslt = &ignore;
      else {
         tmplftm.cur_status = InUse;
         tmplftm.lifetime = lifetime;
         rslt = tmp_loc(alc_tmp(1, &tmplftm));
         }
      }
   else if (rslt->loc_type == V_Temp)
      tmp_status[rslt->u.tmp] = InUse;
   return rslt;
   }

/*
 * mk_goto - make a code structure for goto label
 */
struct code *mk_goto(label)
struct code *label;
   {
   register struct code *cd;

   cd = NewCode(1);    /* # fields == # fields of C_RetSig & C_Break */
   cd->cd_id = C_Goto;
   cd->next = NULL;
   cd->prev = NULL;
   cd->Lbl = label;
   ++label->RefCnt;
   return cd;
   }

/*
 * mk_cpyval - make code to copy a value from one location to another.
 */
static struct code *mk_cpyval(loc1, loc2)
struct val_loc *loc1;
struct val_loc *loc2;
   {
   struct code *cd;

   cd = alc_ary(4);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) = loc1;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) = " = ";
   cd->ElemTyp(2) = A_ValLoc;
   cd->ValLoc(2) = loc2;
   cd->ElemTyp(3) = A_Str;
   cd->Str(3) = ";";
   return cd;
   }

/*
 * asgn_null - make code to assign the null value to a location.
 */
static struct code *asgn_null(loc1)
struct val_loc *loc1;
   {
   struct code *cd;

   cd = alc_ary(2);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) = loc1;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) = " = nulldesc;";
   return cd;
   }

/*
 * oper_name - create the name for the most general implementation of an Icon
 *   operation.
 */
static char *oper_name(impl)
struct implement *impl;
   {
   char *sbuf;

   sbuf = (char *)alloc((unsigned int)(strlen(impl->name) + 5));
   sprintf(sbuf, "%c%c%c_%s", impl->oper_typ, impl->prefix[0], impl->prefix[1],
      impl->name);
   return sbuf;
   }

/*
 * gen_args - generate code to evaluate an argument list.
 */
static struct val_loc *gen_args(n, frst_arg, nargs)
struct node *n;
int frst_arg;
int nargs;
   {
   struct tmplftm *lifetm_ary;
   int i;
   int tmp_indx;

   if (nargs == 0)
      return NULL;

   lifetm_ary = alc_lftm(nargs, &n->n_field[frst_arg]);
   tmp_indx = alc_tmp(nargs, lifetm_ary);
   for (i = 0; i < nargs; ++i)
      gencode(n->n_field[frst_arg + i].n_ptr, tmp_loc(tmp_indx + i));
   free((char *)lifetm_ary);
   return tmp_loc(tmp_indx);
   }

/*
 * gen_case - generate code for a case expression.
 */
static struct val_loc *gen_case(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct node *control;
   struct node *cases;
   struct node *deflt;
   struct node *clause;
   struct val_loc *r1;
   struct val_loc *r2;
   struct val_loc *r3;
   struct code *cd;
   struct code *cd1;
   struct code *fail_sav;
   struct code *skp_lbl;
   struct code *cd_lbl;
   struct code *end_lbl;
   struct c_fnc *fnc_sav;
   struct c_fnc *succ_cont = NULL;

   control = Tree0(n);
   cases = Tree1(n);
   deflt = Tree2(n);

   /*
    * The control clause is bounded.
    */
   r1 = chk_alc(NULL, n); 
   bound(control, r1, 0);

   /*
    * Remember the context in which the case expression occurs and
    *  establish a label at the end of the expression.
    */
   fail_sav = on_failure;
   fnc_sav = cur_fnc;
   end_lbl = alc_lbl("end case", 0);
   cd_add(end_lbl);
   cur_fnc->cursor = end_lbl->prev; /* generate code before the end label */

   /*
    * All cases share the result location of the case expression.
    */
   rslt = chk_alc(rslt, n->lifetime);
   r2 = chk_alc(NULL, n);      /* for result of selection clause */
   r3 = chk_alc(NULL, n);      /* for dereferenced result of control clause */

   while (cases != NULL) {
      /*
       * See if we are at the end of the case clause list.
       */
      if (cases->n_type == N_Ccls) {
         clause = cases;
         cases = NULL;
         }
      else {
         clause = Tree1(cases);
         cases = Tree0(cases);
         }

      /*
       * If the evaluation of the selection code or the comparison of
       *  its value to the control clause fail, execution will proceed
       *  to the "skip clause" label and on to the next case.
       */
      skp_lbl = alc_lbl("skip clause", 0);
      on_failure = skp_lbl;
      cd_add(skp_lbl);
      cur_fnc->cursor = skp_lbl->prev;  /* generate code before end label */

      /*
       * Bound the selection code for this clause.
       */
      cd_lbl = alc_lbl("selected code", Bounding);
      cd_add(cd_lbl);
      cur_fnc->cursor = cd_lbl->prev; 
      gencode(Tree0(clause), r2);

      /*
       * Dereference the results of the control clause and the selection
       *  clause and compare them.
       */
      setloc(clause);
      deref_cd(r1, r3);
      deref_cd(r2, r2);
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(5);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) =                 "!equiv(&";
      cd1->ElemTyp(1) = A_ValLoc;
      cd1->ValLoc(1) =              r3;
      cd->Cond = cd1;
      cd1->ElemTyp(2) = A_Str;
      cd1->Str(2) =                 ", &";
      cd1->ElemTyp(3) = A_ValLoc;
      cd1->ValLoc(3) =              r2;
      cd1->ElemTyp(4) = A_Str;
      cd1->Str(4) =                 ")";
      cd->ThenStmt = sig_cd(on_failure, cur_fnc); 
      cd_add(cd);
      cd_add(sig_cd(cd_lbl, cur_fnc));  /* transfer control to bounding label */

      /*
       * Generate code for the body of this clause after the bounding label.
       */
      cur_fnc = fnc_sav;
      cur_fnc->cursor = cd_lbl;
      on_failure = fail_sav;
      gencode(Tree1(clause), rslt);

      /*
       * If this clause is a generator, call the success continuation
       *  for the case expression, otherwise branch to the end of the
       *  expression.
       */
      if (cur_fnc->cursor->next != skp_lbl) {
         if (succ_cont == NULL)
            succ_cont = alc_fnc(); /* allocate a continuation function */
         callc_add(succ_cont);
         cur_fnc = fnc_sav;
         }
      else
         cd_add(mk_goto(end_lbl));

      /*
       * The code for the next clause goes after the  "skip" label of
       *   this clause.
       */
      cur_fnc->cursor = skp_lbl;
      }

   if (deflt == NULL)
      cd_add(sig_cd(fail_sav, cur_fnc));     /* default action is failure */
   else {
      /*
       * There is an explicit default action.
       */
      on_failure = fail_sav;
      gencode(deflt, rslt);
      if (cur_fnc->cursor->next != end_lbl) {
         if (succ_cont == NULL)
            succ_cont = alc_fnc();
         callc_add(succ_cont);
         cur_fnc = fnc_sav;
         }
      }
   cur_fnc->cursor = end_lbl;

   /*
    * If some clauses are generators but others have transferred control
    *  to here, we must call the success continuation of the case
    *  expression and generate subsequent code there.
    */
   if (succ_cont != NULL) {
      on_failure = fail_sav;
      callc_add(succ_cont);
      cur_fnc = succ_cont;
      on_failure = &resume;
      }
   return rslt;
   }

/*
 * gen_creat - generate code to create a co-expression.
 */
static struct val_loc *gen_creat(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct code *cd;
   struct code *fail_sav;
   struct code *fail_lbl;
   struct c_fnc *fnc_sav;
   struct c_fnc *fnc;
   struct val_loc *co_rslt;
   struct freetmp *ft;
   char sav_prfx[PrfxSz];
   int *tmp_sv;
   int *itmp_sv;
   int *dtmp_sv;
   int *sbuf_sv;
   int *cbuf_sv;
   int ntmp_sv;
   int nitmp_sv;
   int ndtmp_sv;
   int nsbuf_sv;
   int ncbuf_sv;
   int stat_sz_sv;
   int istat_sz_sv;
   int dstat_sz_sv;
   int sstat_sz_sv;
   int cstat_sz_sv;
   int i;


   rslt = chk_alc(rslt, n->lifetime);

   fail_sav = on_failure;
   fnc_sav = cur_fnc;
   for (i = 0; i < PrfxSz; ++i)
      sav_prfx[i] = frm_prfx[i];

   /*
    * Temporary variables are allocated independently for the co-expression.
    */
   tmp_sv = tmp_status;
   itmp_sv = itmp_status;
   dtmp_sv = dtmp_status;
   sbuf_sv = sbuf_status;
   cbuf_sv = cbuf_status;
   stat_sz_sv = status_sz;
   istat_sz_sv = istatus_sz;
   dstat_sz_sv = dstatus_sz;
   sstat_sz_sv = sstatus_sz;
   cstat_sz_sv = cstatus_sz;
   ntmp_sv = num_tmp;
   nitmp_sv = num_itmp;
   ndtmp_sv = num_dtmp;
   nsbuf_sv = num_sbuf;
   ncbuf_sv = num_cbuf;
   tmp_status = (int *)alloc((unsigned int)(status_sz * sizeof(int)));
   itmp_status = (int *)alloc((unsigned int)(istatus_sz * sizeof(int)));
   dtmp_status = (int *)alloc((unsigned int)(dstatus_sz * sizeof(int)));
   sbuf_status = (int *)alloc((unsigned int)(sstatus_sz * sizeof(int)));
   cbuf_status = (int *)alloc((unsigned int)(cstatus_sz * sizeof(int)));
   for (i = 0; i < status_sz; ++i)
      tmp_status[i] = NotAlloc;
   for (i = 0; i < istatus_sz; ++i)
      itmp_status[i] = NotAlloc;
   for (i = 0; i < dstatus_sz; ++i)
      dtmp_status[i] = NotAlloc;
   for (i = 0; i < sstatus_sz; ++i)
      sbuf_status[i] = NotAlloc;
   for (i = 0; i < cstatus_sz; ++i)
      cbuf_status[i] = NotAlloc;
   num_tmp = 0;
   num_itmp = 0;
   num_dtmp = 0;
   num_sbuf = 0;
   num_cbuf = 0;

   /*
    * Put code for co-expression in separate function. We will need a new
    *  type of procedure frame which contains copies of local variables,
    *  copies of arguments, and temporaries for use by the co-expression.
    */
   fnc = alc_fnc();
   fnc->ref_cnt = 1;
   fnc->flag |= CF_Coexpr;
   ChkPrefix(fnc->prefix);
   for (i = 0; i < PrfxSz; ++i)
      frm_prfx[i] = fnc->frm_prfx[i] = fnc->prefix[i];
   cur_fnc = fnc;

   /*
    * Set up a co-expression failure label followed by a context switch
    *  and a branch back to the failure label.
    */
   fail_lbl = alc_lbl("co_fail", 0);
   cd_add(fail_lbl);
   lastline = 0;  /* force setting line number so tracing matches interp */
   setloc(n);
   cd = alc_ary(2);
   cd->ElemTyp(0) = A_Str;
   cd->ElemTyp(1) = A_Str;
   cd->Str(0) = "co_chng(popact((struct b_coexpr *)BlkLoc(k_current)),";
   cd->Str(1) =    "NULL, NULL, A_Cofail, 1);";
   cd_add(cd);
   cd_add(mk_goto(fail_lbl));
   cur_fnc->cursor = fail_lbl->prev;  /* code goes before failure label */
   on_failure = fail_lbl;

   /*
    * Generate code for the co-expression body, using the same
    *  dereferencing rules as for procedure return.
    */
   lastfiln = "";  /* force setting of file name and line number */
   lastline = 0;
   setloc(n);
   ++create_lvl;
   co_rslt = genretval(n, Tree0(n), NULL);
   --create_lvl;

   /*
    * If the co-expression might produce a result, generate a co-expression
    *  context switch.
    */
   if (co_rslt != NULL) {
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) = "++BlkLoc(k_current)->coexpr.size;";
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) = "co_chng(popact((struct b_coexpr *)BlkLoc(k_current)), &";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) = co_rslt;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) = ", NULL, A_Coret, 1);";
      cd_add(cd);
      cd_add(sig_cd(on_failure, cur_fnc)); /* if reactivated, resume expr */
      }

   /*
    * Output the new frame definition.
    */
   prt_frame(frm_prfx, cur_proc->tnd_loc + num_tmp + Abs(cur_proc->nargs),
      num_itmp, num_dtmp, num_sbuf, num_cbuf);

   /*
    * Now return to original function and produce code to create the
    *  co-expression.
    */
   cur_fnc = fnc_sav;
   for (i = 0; i < PrfxSz; ++i)
      frm_prfx[i] = sav_prfx[i];
   on_failure = fail_sav;

   lastfiln = "";  /* force setting of file name and line number */
   lastline = 0;
   setloc(n);
   cd = NewCode(5);
   cd->cd_id =  C_Create;
   cd->Rslt = rslt;
   cd->Cont = fnc;
   cd->NTemps = num_tmp;
   cd->WrkSize = num_itmp;
   cd->NextCreat = cur_fnc->creatlst;
   cur_fnc->creatlst = cd;
   cd_add(cd);

   /*
    * Restore arrays for temporary variable allocation.
    */
   free((char *)tmp_status);
   free((char *)itmp_status);
   free((char *)dtmp_status);
   free((char *)sbuf_status);
   free((char *)cbuf_status);
   tmp_status = tmp_sv;
   itmp_status = itmp_sv;
   dtmp_status = dtmp_sv;
   sbuf_status = sbuf_sv;
   cbuf_status = cbuf_sv;
   status_sz = stat_sz_sv;
   istatus_sz = istat_sz_sv;
   dstatus_sz = dstat_sz_sv;
   sstatus_sz = sstat_sz_sv;
   cstatus_sz = cstat_sz_sv;
   num_tmp = ntmp_sv;
   num_itmp = nitmp_sv;
   num_dtmp = ndtmp_sv;
   num_sbuf = nsbuf_sv;
   num_cbuf = ncbuf_sv;

   /*
    * Temporary variables that exist to the end of the co-expression
    *   have no meaning in the surrounding code and must not be
    *   deallocated there.
    */
   while (n->freetmp != NULL) {
      ft = n->freetmp->next;
      n->freetmp->next = freetmp_pool;
      freetmp_pool = n->freetmp;
      n->freetmp = ft;
      }

   return rslt;
   }

/*
 * gen_lim - generate code for limitation.
 */
static struct val_loc *gen_lim(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct node *expr;
   struct node *limit;
   struct val_loc *lim_desc;
   struct code *cd;
   struct code *cd1;
   struct code *lbl;
   struct code *fail_sav;
   struct c_fnc *fnc_sav;
   struct c_fnc *succ_cont;
   struct val_loc *lim_int;
   struct lentry *single;
   int deref;

   expr = Tree0(n);
   limit = Tree1(n);

   /*
    * Generate code to compute the limitation value and dereference it.
    */
   deref = HasVar(varsubtyp(limit->type, &single));
   if (single != NULL) {
      /*
       * Limitation is in a named variable. Use value directly from
       *  the variable rather than saving the result of the expression.
       */
      gencode(limit, &ignore);
      lim_desc = var_ref(single);
      }
   else {
      lim_desc = gencode(limit, NULL);
      if (deref)
         deref_cd(lim_desc, lim_desc);
      }

   setloc(n);
   fail_sav = on_failure;

   /*
    * Try to convert the limitation value into an integer.
    */
   lim_int = itmp_loc(alc_itmp(n->intrnl_lftm));
   cur_symtyps = n->symtyps;
   if (largeints || (eval_is(int_typ, 0) & MaybeFalse)) {
      /*
       * Must call the conversion routine.
       */
      lbl = alc_lbl("limit is int", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* conversion goes before label */
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(5);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) =                  "cnv_c_int(&";
      cd1->ElemTyp(1) = A_ValLoc;
      cd1->ValLoc(1) =               lim_desc;
      cd1->ElemTyp(2) = A_Str;
      cd1->Str(2) =                  ", &";
      cd1->ElemTyp(3) = A_ValLoc;
      cd1->ValLoc(3) =                lim_int;
      cd1->ElemTyp(4) = A_Str;
      cd1->Str(4) =                  ")";
      cd->Cond = cd1;
      cd->ThenStmt = mk_goto(lbl);
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                  "err_msg(101, &";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =               lim_desc;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                  ");";
      cd_add(cd);
      if (err_conv)
         cd_add(sig_cd(on_failure, cur_fnc));
      cur_fnc->cursor = lbl;
      }
   else {
      /*
       * The C integer is in the vword.
       */
      cd = alc_ary(4);
      cd->ElemTyp(0) = A_ValLoc;
      cd->ValLoc(0) =                lim_int;
      cd->ElemTyp(1) = A_Str;
      cd->Str(1) =                   " = IntVal(";
      cd->ElemTyp(2) = A_ValLoc;
      cd->ValLoc(2) =                lim_desc;
      cd->ElemTyp(3) = A_Str;
      cd->Str(3) =                   ");";
      cd_add(cd);
      }

   /*
    * Make sure the limitation value is positive.
    */
   lbl = alc_lbl("limit positive", 0);
   cd_add(lbl);
   cur_fnc->cursor = lbl->prev;        /* code goes before label */
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(2);
   cd1->ElemTyp(0) = A_ValLoc;
   cd1->ValLoc(0) =                lim_int;
   cd1->ElemTyp(1) = A_Str;
   cd1->Str(1) =                  " >= 0";
   cd->Cond = cd1;
   cd->ThenStmt = mk_goto(lbl);
   cd_add(cd);
   cd = alc_ary(3);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                  "err_msg(205, &";
   cd->ElemTyp(1) = A_ValLoc;
   cd->ValLoc(1) =               lim_desc;
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =                  ");";
   cd_add(cd);
   if (err_conv)
      cd_add(sig_cd(on_failure, cur_fnc));
   cur_fnc->cursor = lbl;

   /*
    * If the limitation value is 0, fail immediately.
    */
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(2);
   cd1->ElemTyp(0) = A_ValLoc;
   cd1->ValLoc(0) =              lim_int;
   cd1->ElemTyp(1) = A_Str;
   cd1->Str(1) =                  " == 0";
   cd->Cond = cd1;
   cd->ThenStmt = sig_cd(on_failure, cur_fnc);
   cd_add(cd);

   /*
    * Establish where to go when limit has been reached.
    */
   fnc_sav = cur_fnc;
   lbl = alc_lbl("limit", 0);
   cd_add(lbl);
   cur_fnc->cursor = lbl->prev;  /* limited expression goes before label */

   /*
    * Generate code for limited expression and to check the limit value.
    */
   rslt = gencode(expr, rslt);
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(3);
   cd1->ElemTyp(0) = A_Str;
   cd1->Str(0) =                  "--";
   cd1->ElemTyp(1) = A_ValLoc;
   cd1->ValLoc(1) =               lim_int;
   cd1->ElemTyp(2) = A_Str;
   cd1->Str(2) =                  " == 0";
   cd->Cond = cd1;
   cd->ThenStmt = sig_cd(lbl, cur_fnc);
   cd_add(cd);

   /*
    * Call the success continuation both here and after the limitation
    *  label.
    */
   succ_cont = alc_fnc();
   callc_add(succ_cont);
   cur_fnc = fnc_sav;
   cur_fnc->cursor = lbl;
   on_failure = fail_sav;
   callc_add(succ_cont);
   cur_fnc = succ_cont;
   on_failure = &resume;

   return rslt;
   }

/*
 * gen_apply - generate code for the apply operator, !.
 */
static struct val_loc *gen_apply(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct val_loc *callee;
   struct val_loc *lst;
   struct code *arg_lst;
   struct code *on_ret;
   struct c_fnc *fnc;

   /*
    * Generate code to compute the two operands.
    */
   callee = gencode(Tree0(n), NULL);
   lst = gencode(Tree1(n), NULL);
   rslt = chk_alc(rslt, n->lifetime);
   setloc(n);

   /*
    * Construct argument list for apply().
    */
   arg_lst = alc_ary(6);
   arg_lst->ElemTyp(0) = A_Str;
   arg_lst->Str(0) =                   "&";
   arg_lst->ElemTyp(1) = A_ValLoc;
   arg_lst->ValLoc(1) =                callee;
   arg_lst->ElemTyp(2) = A_Str;
   arg_lst->Str(2) =                   ", &";
   arg_lst->ElemTyp(3) = A_ValLoc;
   arg_lst->ValLoc(3) =                lst;
   arg_lst->ElemTyp(4) = A_Str;
   arg_lst->Str(4) =                   ", &";
   arg_lst->ElemTyp(5) = A_ValLoc;
   arg_lst->ValLoc(5) =                rslt;

   /*
    * Generate code to call apply(). Assume the operation can suspend and
    *   allocate a continuation. If it returns a "continue" signal,
    *   just break out of the signal handling code and fall into a call
    *   to the continuation.
    */
   on_ret = NewCode(1);      /* #fields for C_Break == #fields for C_Goto */
   on_ret->cd_id = C_Break;
   on_ret->next = NULL;
   on_ret->prev = NULL;
   fnc = alc_fnc();          /* success continuation */
   callo_add("apply", DoesFail | DoesRet | DoesSusp, fnc, 1, arg_lst, on_ret);
   callc_add(fnc);
   cur_fnc = fnc;            /* subsequent code goes in the continuation */
   on_failure = &resume;

   return rslt;
   }


/*
 * gen_scan - generate code for string scanning.
 */
static struct val_loc *gen_scan(n, rslt)
nodeptr n;
struct val_loc *rslt;
   {
   struct node *op;
   struct node *subj;
   struct node *body;
   struct scan_info *scanp;
   struct val_loc *asgn_var;
   struct val_loc *new_subj;
   struct val_loc *scan_rslt;
   struct tmplftm *lifetm_ary;
   struct lentry *subj_single;
   struct lentry *body_single;
   struct code *cd;
   struct code *cd1;
   struct code *lbl;
   struct implement *impl;
   int subj_deref;
   int body_deref;
   int op_tok;
   int tmp_indx;

   op = Tree0(n);          /* operator node '?' or '?:=' */
   subj = Tree1(n);        /* subject expression */
   body = Tree2(n);        /* scanning expression */
   op_tok = optab[Val0(op)].tok.t_type;

   /*
    * The location of the save areas for scanning environments is stored
    *  in list so they can be accessed by expressions that transfer
    *  control out of string scanning. Get the next list element and
    *  allocate the save areas in the procedure frame.
    */
   scanp = nxt_scan;
   if (nxt_scan->next == NULL)
      nxt_scan->next = NewStruct(scan_info);
   nxt_scan = nxt_scan->next;
   scanp->outer_sub = chk_alc(NULL, n->intrnl_lftm);
   scanp->outer_pos = itmp_loc(alc_itmp(n->intrnl_lftm));
   scanp->inner_sub = chk_alc(NULL, n->intrnl_lftm); 
   scanp->inner_pos = itmp_loc(alc_itmp(n->intrnl_lftm));

   subj_deref = HasVar(varsubtyp(subj->type, &subj_single));
   if (subj_single != NULL) {
      /*
       * The subject value is in a named variable. Use value directly from
       *  the variable rather than saving the result of the expression.
       */
      gencode(subj, &ignore);
      new_subj = var_ref(subj_single);

      if (op_tok == AUGQMARK) {
         body_deref = HasVar(varsubtyp(body->type, &body_single));
         if (body_single != NULL)
            scan_rslt = &ignore; /* we know where the value will be */
         else
            scan_rslt = chk_alc(NULL, n->intrnl_lftm);
         }
      else
         scan_rslt = rslt; /* result of 2nd operand is result of scanning */
      }
   else if (op_tok == AUGQMARK) {
      /*
       * Augmented string scanning using general assignment. The operands
       *  must be in consecutive locations.
       */
      lifetm_ary = alc_lftm(2, &n->n_field[1]);
      tmp_indx = alc_tmp(2, lifetm_ary);
      asgn_var = tmp_loc(tmp_indx++);
      scan_rslt = tmp_loc(tmp_indx);
      free((char *)lifetm_ary);

      gencode(subj, asgn_var);
      new_subj = chk_alc(NULL, n->intrnl_lftm);
      deref_cd(asgn_var, new_subj);
      }
   else {
      new_subj = gencode(subj, NULL);
      if (subj_deref)
          deref_cd(new_subj, new_subj);
      scan_rslt = rslt; /* result of 2nd operand is result of scanning */
      }

   /*
    * Produce code to save the old scanning environment.
    */
   setloc(op);
   save_env(scanp->outer_sub, scanp->outer_pos);

   /*
    * Produce code to handle failure of the body of string scanning.
    */
   lbl = alc_lbl("scan fail", 0);
   cd_add(lbl);
   restr_env(scanp->outer_sub, scanp->outer_pos);
   cd_add(sig_cd(on_failure, cur_fnc)); /* fail */
   cur_fnc->cursor = lbl->prev;         /* body goes before label */
   on_failure = lbl;

   /*
    * If necessary, try to convert the subject to a string. Note that if
    *   error conversion occurs, backtracking will restore old subject.
    */
   cur_symtyps = n->symtyps;
   if (eval_is(str_typ, 0) & MaybeFalse) {
      lbl = alc_lbl("&subject is string", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(3);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) =                 "cnv_str(&";
      cd1->ElemTyp(1) = A_ValLoc;
      cd1->ValLoc(1) =              new_subj;
      cd1->ElemTyp(2) = A_Str;
      cd1->Str(2) =                 ", &k_subject)";
      cd->Cond = cd1;
      cd->ThenStmt = mk_goto(lbl);
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                  "err_msg(103, &";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =               new_subj;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                  ");";
      cd_add(cd);
      if (err_conv)
         cd_add(sig_cd(on_failure, cur_fnc));
      cur_fnc->cursor = lbl;
      }
   else {
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                  "k_subject = ";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =               new_subj;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                  ";";
      cd_add(cd);
      }
   cd = alc_ary(1);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                 "k_pos = 1;";
   cd_add(cd);

   scan_rslt = gencode(body, scan_rslt);

   setloc(op);
   if (op_tok == AUGQMARK) {
      /*
       * '?:=' - perform assignment.
       */
      if (subj_single != NULL) {
         /*
          * Assignment to a named variable.
          */
         if (body_single != NULL)
            cd_add(mk_cpyval(new_subj, var_ref(body_single)));
         else if (body_deref)
            deref_cd(scan_rslt, new_subj);
         else
             cd_add(mk_cpyval(new_subj, scan_rslt));
         }
      else {
         /*
          * Use general assignment.
          */
         impl = optab[asgn_loc].binary;
         if (impl == NULL) {
            nfatal(op, "assignment not implemented", NULL);
            rslt = &ignore; /* make sure code generation can continue */
            }
         else {
            implproto(impl);
            rslt = chk_alc(rslt, n->lifetime);
            mk_callop(oper_name(impl), impl->ret_flag, asgn_var, 2, rslt,0);
            }
         }
      }
   else {
      /*
       * '?'
       */
      rslt = scan_rslt;
      }

   /*
    * Produce code restore subject and pos when the body of the
    *  scanning expression succeeds. The new subject and pos must
    *  be saved in case of resumption.
    */
   save_env(scanp->inner_sub, scanp->inner_pos);
   restr_env(scanp->outer_sub, scanp->outer_pos);

   /*
    * Produce code to handle resumption of string scanning.
    */
   lbl = alc_lbl("scan resume", 0);
   cd_add(lbl);
   save_env(scanp->outer_sub, scanp->outer_pos);
   restr_env(scanp->inner_sub, scanp->inner_pos);
   cd_add(sig_cd(on_failure, cur_fnc)); /* fail */
   cur_fnc->cursor = lbl->prev;     /* success continuation goes before label */
   on_failure = lbl;

   nxt_scan = scanp;
   return rslt;
   }

/*
 * gen_act - generate code for co-expression activation.
 */
static struct val_loc *gen_act(n, rslt)
nodeptr n;
struct val_loc *rslt;
   {
   struct node *op;
   struct node *transmit;
   struct node *coexpr;
   struct tmplftm *lifetm_ary;
   struct val_loc *trans_loc;
   struct val_loc *coexpr_loc;
   struct val_loc *asgn1;
   struct val_loc *asgn2;
   struct val_loc *act_rslt;
   struct lentry *c_single;
   struct code *cd;
   struct code *cd1;
   struct code *lbl;
   struct implement *impl;
   int c_deref;
   int op_tok;
   int tmp_indx;

   op = Tree0(n);        /* operator node for '@' or '@:=' */
   transmit = Tree1(n);  /* expression for value to transmit */
   coexpr = Tree2(n);    /* expression for co-expression */
   op_tok = optab[Val0(op)].tok.t_type;

   /*
    * Produce code for the value to be transmitted.
    */
   if (op_tok == AUGAT) {
      /*
       * Augmented activation. This is seldom used so don't try too
       *  hard to optimize it. Allocate contiguous temporaries for
       *  the operands to the assignment.
       */
      lifetm_ary = alc_lftm(2, &n->n_field[1]);
      tmp_indx = alc_tmp(2, lifetm_ary);
      asgn1 = tmp_loc(tmp_indx++);
      asgn2 = tmp_loc(tmp_indx);
      free((char *)lifetm_ary);

      /*
       * Generate code to produce the left-hand-side of the assignment.
       *  This is also the transmitted value. Activation may need a
       *  dereferenced value, so this must be in a different location.
       */
      gencode(transmit, asgn1);
      trans_loc = chk_alc(NULL, n->intrnl_lftm);
      setloc(op);
      deref_ret(asgn1, trans_loc, varsubtyp(transmit->type, NULL));
      }
   else
      trans_loc = genretval(op, transmit, NULL); /* ordinary activation */

   /*
    * Determine if the value to be activated needs dereferencing, and
    *  see if it can only come from a single named variable.
    */
   c_deref = HasVar(varsubtyp(coexpr->type, &c_single));
   if (c_single == NULL) {
      /*
       * The value is something other than a single named variable.
       */
      coexpr_loc = gencode(coexpr, NULL);
      if (c_deref)
         deref_cd(coexpr_loc, coexpr_loc);
      }
   else {
      /*
       * The value is in a named variable. Use it directly from the
       *  variable rather than saving the result of the expression.
       */
      gencode(coexpr, &ignore);
      coexpr_loc = var_ref(c_single);
      }

   /*
    * Make sure the value to be activated is a co-expression. Perform
    *   run-time checking if necessary.
    */
   cur_symtyps = n->symtyps;
   if (eval_is(coexp_typ, 1) & MaybeFalse) {
      lbl = alc_lbl("is co-expression", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(3);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) =                  "(";
      cd1->ElemTyp(1) = A_ValLoc;
      cd1->ValLoc(1) =               coexpr_loc;
      cd1->ElemTyp(2) = A_Str;
      cd1->Str(2) =                  ").dword == D_Coexpr";
      cd->Cond = cd1;
      cd->ThenStmt = mk_goto(lbl);
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                   "err_msg(118, &(";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =                coexpr_loc;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                   "));";
      cd_add(cd);
      if (err_conv)
         cd_add(sig_cd(on_failure, cur_fnc));
      cur_fnc->cursor = lbl;
      }

   /*
    * Make sure a result location has been allocated. For ordinary
    *  activation, this is where activate() puts its result. For
    *  augmented activation, this is where assignment puts its result.
    */
   rslt = chk_alc(rslt, n->lifetime);
   if (op_tok == AUGAT)
      act_rslt = asgn2;
   else
      act_rslt = rslt;

   /*
    * Generate code to call activate().
    */
   setloc(n);
   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(7);
   cd1->ElemTyp(0) = A_Str;
   cd1->Str(0) =                  "activate(&";
   cd1->ElemTyp(1) = A_ValLoc;
   cd1->ValLoc(1) =                trans_loc;
   cd1->ElemTyp(2) = A_Str;
   cd1->Str(2) =                  ", (struct b_coexpr *)BlkLoc(";
   cd1->ElemTyp(3) = A_ValLoc;
   cd1->ValLoc(3) =                coexpr_loc;
   cd1->ElemTyp(4) = A_Str;
   cd1->Str(4) =                  "), &";
   cd1->ElemTyp(5) = A_ValLoc;
   cd1->ValLoc(5) =                act_rslt;
   cd1->ElemTyp(6) = A_Str;
   cd1->Str(6) =                  ") == A_Resume";
   cd->Cond = cd1;
   cd->ThenStmt = sig_cd(on_failure, cur_fnc);
   cd_add(cd);

   /*
    * For augmented activation, generate code to call assignment.
    */
   if (op_tok == AUGAT) {
      impl = optab[asgn_loc].binary;
      if (impl == NULL) {
         nfatal(op, "assignment not implemented", NULL);
         rslt = &ignore; /* make sure code generation can continue */
         }
      else {
         implproto(impl);
         mk_callop(oper_name(impl), impl->ret_flag, asgn1, 2, rslt, 0);
         }
      }

   return rslt;
   }

/*
 * save_env - generate code to save scanning environment.
 */
static void save_env(sub_sav, pos_sav)
struct val_loc *sub_sav;
struct val_loc *pos_sav;
   {
   struct code *cd;

   cd = alc_ary(2);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) =              sub_sav;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) =                 " = k_subject;";
   cd_add(cd);
   cd = alc_ary(2);
   cd->ElemTyp(0) = A_ValLoc;
   cd->ValLoc(0) =              pos_sav;
   cd->ElemTyp(1) = A_Str;
   cd->Str(1) =                 " = k_pos;";
   cd_add(cd);
   }

/*
 * restr_env - generate code to restore scanning environment.
 */
static void restr_env(sub_sav, pos_sav)
struct val_loc *sub_sav;
struct val_loc *pos_sav;
   {
   struct code *cd;

   cd = alc_ary(3);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                 "k_subject = ";
   cd->ElemTyp(1) = A_ValLoc;
   cd->ValLoc(1) =              sub_sav;
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =                 ";";
   cd_add(cd);
   cd = alc_ary(3);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                 "k_pos = ";
   cd->ElemTyp(1) = A_ValLoc;
   cd->ValLoc(1) =              pos_sav;
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =                 ";";
   cd_add(cd);
   }

/*
 * mk_callop - produce the code to directly call an operation.
 */
static void mk_callop(oper_nm, ret_flag, arg1rslt, nargs, rslt, optim)
char *oper_nm;
int ret_flag;
struct val_loc *arg1rslt;
int nargs;
struct val_loc *rslt;
int optim;
   {
   struct code *arg_lst;
   struct code *on_ret;
   struct c_fnc *fnc;
   int n;
   int need_cont;

   /*
    * If this operation can return an "continue" signal, we will need
    *   a break statement in the signal switch to handle it.
    */
   if (ret_flag & DoesRet) {
      on_ret = NewCode(1);      /* #fields == #fields C_Goto */
      on_ret->cd_id = C_Break;
      on_ret->next = NULL;
      on_ret->prev = NULL;
      }
   else
      on_ret = NULL;

   /*
    * Construct argument list for the C function implementing the
    *  operation. First compute the size of the code array for the
    *  argument list; this varies if we are using an optimized calling
    *  interface.
    */
   if (optim) {
      n = 0;
      if (arg1rslt != NULL)
         n += 2;
      if (ret_flag & (DoesRet | DoesSusp)) {
         if (n > 0)
            ++n;
         n += 2;
         }
      }
   else
      n = 7;
   if (n == 0)
      arg_lst = NULL;
   else {
      arg_lst = alc_ary(n);
      n = 0;
      if (!optim) {
         arg_lst->ElemTyp(n) = A_Intgr;       /* number of arguments */
         arg_lst->Intgr(n) = nargs;
         ++n;
         arg_lst->ElemTyp(n) = A_Str;        /* , */
         arg_lst->Str(n) = ", ";
         ++n;
         }
      if (arg1rslt == NULL) {             /* location of first argument */
         if (!optim) {
            arg_lst->ElemTyp(n) = A_Str;
            arg_lst->Str(n) = "NULL";
            ++n;
            arg_lst->ElemTyp(n) = A_Str;
            arg_lst->Str(n) = "";         /* nothing, but must fill slot */
            ++n;
            }
         }
      else {
         arg_lst->ElemTyp(n) = A_Str;
         arg_lst->Str(n) = "&";
         ++n;
         arg_lst->ElemTyp(n) = A_ValLoc;
         arg_lst->ValLoc(n) = arg1rslt;
         ++n;
         }
      if (!optim || ret_flag & (DoesRet | DoesSusp)) {
         if (n > 0) {
            arg_lst->ElemTyp(n) = A_Str;        /* , */
            arg_lst->Str(n) = ", ";
            ++n;
            }
         arg_lst->ElemTyp(n) = A_Str;        /* location of result */
         arg_lst->Str(n) = "&";
         ++n;
         arg_lst->ElemTyp(n) = A_ValLoc;
         arg_lst->ValLoc(n) = rslt;
         }
      }

   /*
    * Generate code to call the operation and handle returned signals.
    */
   if (ret_flag & DoesSusp) {
      /*
       * The operation suspends, so call it with a continuation, then
       *  proceed to generate code in the continuation.
       */
      fnc = alc_fnc();
      callo_add(oper_nm, ret_flag, fnc, 1, arg_lst, on_ret);
      if (ret_flag & DoesRet)
         callc_add(fnc);
      cur_fnc = fnc;
      on_failure = &resume;
      }
   else {
      /*
       * No continuation is needed, but if standard calling conventions
       *  are used, a NULL continuation argument is required.
       */
      if (optim)
         need_cont = 0;
      else
         need_cont = 1;
      callo_add(oper_nm, ret_flag, NULL, need_cont, arg_lst, on_ret);
      }
}

/*
 * genretval - generate code for the expression in a return/suspend or
 *  for the expression for the value to be transmitted in a co-expression
 *  context switch.
 */
static struct val_loc *genretval(n, expr, dest)
struct node *n;
struct node *expr;
struct val_loc *dest;
   {
   int subtypes;
   struct lentry *single;
   struct val_loc *val;

   subtypes = varsubtyp(expr->type, &single);

   /*
    * If we have a single local or argument, we don't need to construct
    *  a variable reference; we need the value and we know where it is.
    */
   if (single != NULL && (subtypes & (HasLcl | HasPrm))) {
      gencode(expr, &ignore);
      val = var_ref(single);
      if (dest == NULL)
         dest = val;
      else
         cd_add(mk_cpyval(dest, val));
      }
   else {
      dest = gencode(expr, dest);
      setloc(n);
      deref_ret(dest, dest, subtypes);
      }

   return dest;
   }

/*
 * deref_ret - produced dereferencing code for values returned from
 *  procedures or transmitted to co-expressions.
 */
static void deref_ret(src, dest, subtypes)
struct val_loc *src;
struct val_loc *dest;
int subtypes;
   {
   struct code *cd;
   struct code *lbl;

   if (src == NULL)
      return;  /* no value to dereference */

   /*
    * If there may be values that do not need dereferencing, insure that the
    *  values are in the destination and make it the source of dereferencing.
    */
   if ((subtypes & (HasVal | HasGlb)) && (src != dest)) {
      cd_add(mk_cpyval(dest, src));
      src = dest;
      }

   if (subtypes & (HasLcl | HasPrm)) {
      /*
       * Some values may need to be dereferenced.
       */
      lbl = NULL;
      if (subtypes & HasVal) {
         /*
          * We may have a non-variable and must check at run time.
          */
         lbl = check_var(dest, NULL);
         }

      if (subtypes & HasGlb) {
         /*
          * Make sure we don't dereference any globals, use retderef().
          */
         if (subtypes & HasLcl) {
            /*
             * We must dereference any locals.
             */
            cd = alc_ary(3);
            cd->ElemTyp(0) = A_Str;
            cd->Str(0) =                "retderef(&";
            cd->ElemTyp(1) = A_ValLoc;
            cd->ValLoc(1) =             dest;
            cd->ElemTyp(2) = A_Str;
            cd->Str(2) =
               ", (word *)pfp->tend.d, (word *)(pfp->tend.d + pfp->tend.num));";
            cd_add(cd);
            /*
             * We may now have a value. We must check at run-time and skip
             *  any attempt to dereference an argument.
             */
            lbl = check_var(dest, lbl);
            }
   
         if (subtypes & HasPrm) {
            /*
             * We must dereference any arguments.
             */
            cd = alc_ary(5);
            cd->ElemTyp(0) = A_Str;
            cd->Str(0) =                "retderef(&";
            cd->ElemTyp(1) = A_ValLoc;
            cd->ValLoc(1) =             dest;
            cd->ElemTyp(2) = A_Str;
            cd->Str(2) =                ", (word *)glbl_argp, (word *)(glbl_argp + ";
            cd->ElemTyp(3) = A_Intgr;
            cd->Intgr(3) =              Abs(cur_proc->nargs);
            cd->ElemTyp(4) = A_Str;
            cd->Str(4) =                 "));";
            cd_add(cd);
            }
         }
      else /* No globals */
         deref_cd(src, dest);

      if (lbl != NULL)
         cur_fnc->cursor = lbl;   /* continue after label */
      }
   }

/*
 * check_var - generate code to make sure a descriptor contains a variable
 *  reference. If no label is given to jump to for a non-variable, allocate
 *  one and generate code before it.
 */
static struct code *check_var(d, lbl)
struct val_loc *d;
struct code *lbl;
   {
   struct code *cd, *cd1;

   if (lbl == NULL) {
      lbl = alc_lbl("not variable", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      }

   cd = NewCode(2);
   cd->cd_id = C_If;
   cd1 = alc_ary(3);
   cd1->ElemTyp(0) = A_Str;
   cd1->Str(0) =                  "!Var(";
   cd1->ElemTyp(1) = A_ValLoc;
   cd1->ValLoc(1) =               d;
   cd1->ElemTyp(2) = A_Str;
   cd1->Str(2) =                  ")";
   cd->Cond = cd1;
   cd->ThenStmt = mk_goto(lbl);
   cd_add(cd);

   return lbl;
   }

/*
 * field_ref - generate code for a field reference.
 */
static struct val_loc *field_ref(n, rslt)
struct node *n;
struct val_loc *rslt;
   {
   struct node *rec;
   struct node *fld;
   struct fentry *fp;
   struct par_rec *rp;
   struct val_loc *rec_loc;
   struct code *cd;
   struct code *cd1;
   struct code *lbl;
   struct lentry *single;
   int deref;
   int num_offsets;
   int offset;
   int bad_recs;

   rec = Tree0(n);
   fld = Tree1(n);

   /*
    * Generate code to compute the record value and dereference it.
    */
   deref = HasVar(varsubtyp(rec->type, &single));
   if (single != NULL) {
      /*
       * The record is in a named variable. Use value directly from
       *  the variable rather than saving the result of the expression.
       */
      gencode(rec, &ignore);
      rec_loc = var_ref(single);
      }
   else {
      rec_loc = gencode(rec, NULL);
      if (deref)
         deref_cd(rec_loc, rec_loc);
      }

   setloc(fld);

   /*
    * Make sure the operand is a record.
    */
   cur_symtyps = n->symtyps;
   if (eval_is(rec_typ, 0) & MaybeFalse) {
      lbl = alc_lbl("is record", 0);
      cd_add(lbl);
      cur_fnc->cursor = lbl->prev;        /* code goes before label */
      cd = NewCode(2);
      cd->cd_id = C_If;
      cd1 = alc_ary(3);
      cd1->ElemTyp(0) = A_Str;
      cd1->Str(0) =                  "(";
      cd1->ElemTyp(1) = A_ValLoc;
      cd1->ValLoc(1) =               rec_loc;
      cd1->ElemTyp(2) = A_Str;
      cd1->Str(2) =                  ").dword == D_Record";
      cd->Cond = cd1;
      cd->ThenStmt = mk_goto(lbl);
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =                   "err_msg(107, &";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =                rec_loc;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                   ");";
      cd_add(cd);
      if (err_conv)
         cd_add(sig_cd(on_failure, cur_fnc));
      cur_fnc->cursor = lbl;
      }

   rslt = chk_alc(rslt, n->lifetime);

   /*
    * Find the list of records containing this field.
    */
   if ((fp = flookup(Str0(fld))) == NULL) {
      nfatal(n, "invalid field", Str0(fld));
      return rslt;
      }

   /*
    * Generate code for declarations and to get the record block pointer.
    */
   cd = alc_ary(1);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =             "{";
   cd_add(cd);
   cd = alc_ary(3);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =           "struct b_record *r_rp = (struct b_record *) BlkLoc(";
   cd->ElemTyp(1) = A_ValLoc;
   cd->ValLoc(1) =          rec_loc;
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =             ");";
   cd_add(cd);
   if (err_conv) {
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =             "int r_must_fail = 0;";
      cd_add(cd);
      }

   /*
    * Determine which records are in the record type.
    */
   mark_recs(fp, cur_symtyps->types[0], &num_offsets, &offset, &bad_recs);

   /*
    * Generate code to insure that the field belongs to the record
    *  and to index into the record block.
    */
   if (num_offsets == 1 && !bad_recs) {
      /*
       * We already know the offset of the field.
       */
      cd = alc_ary(4);
      cd->ElemTyp(0) = A_ValLoc;
      cd->ValLoc(0) =           rslt;
      cd->ElemTyp(1) = A_Str;
      cd->Str(1) =              ".dword = D_Var + ((word *)&r_rp->fields[";
      cd->ElemTyp(2) = A_Intgr;
      cd->Intgr(2) =            offset;
      cd->ElemTyp(3) = A_Str;
      cd->Str(3) =              "] - (word *)r_rp);";
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "VarLoc(";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =           rslt;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =              ") = (dptr)r_rp;";
      cd_add(cd);
      for (rp = fp->rlist; rp != NULL; rp = rp->next)
         rp->mark = 0;
      }
   else {
      /*
       * The field appears in several records. generate code to determine
       *  which one it is.
       */

      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "dptr r_dp;";
      cd_add(cd);
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "switch (r_rp->recdesc->proc.recnum) {";
      cd_add(cd);

      rp = fp->rlist;
      while (rp != NULL) {
         offset = rp->offset;
         while (rp != NULL && rp->offset == offset) {
            if (rp->mark) {
               rp->mark = 0;
               cd = alc_ary(3);
               cd->ElemTyp(0) = A_Str;
               cd->Str(0) =              "   case ";
               cd->ElemTyp(1) = A_Intgr;
               cd->Intgr(1) =            rp->rec->rec_num;
               cd->ElemTyp(2) = A_Str;
               cd->Str(2) =              ":";
               cd_add(cd);
               }
            rp = rp->next;
            }

         cd = alc_ary(3);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =              "      r_dp = &r_rp->fields[";
         cd->ElemTyp(1) = A_Intgr;
         cd->Intgr(1) =                   offset;
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =                     "];";
         cd_add(cd);
         cd = alc_ary(1);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =              "      break;";
         cd_add(cd);
         }

      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "   default:";
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "      err_msg(207, &";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =                   rec_loc;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =                     ");";
      cd_add(cd);
      if (err_conv) {
         /*
          * The peephole analyzer doesn't know how to handle a goto or return
          *  in a switch statement, so just set a flag here.
          */
         cd = alc_ary(1);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =       "      r_must_fail = 1;";
         cd_add(cd);
         }
      cd = alc_ary(1);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "   }";
      cd_add(cd);
      if (err_conv) {
         /*
          * Now that we are out of the switch statement, see if the flag
          *   was set to indicate error conversion.
          */
         cd = NewCode(2);
         cd->cd_id = C_If;
         cd1 = alc_ary(1);
         cd1->ElemTyp(0) = A_Str;
         cd1->Str(0) =                  "r_must_fail";
         cd->Cond = cd1;
         cd->ThenStmt = sig_cd(on_failure, cur_fnc);
         cd_add(cd);
         }
      cd = alc_ary(2);
      cd->ElemTyp(0) = A_ValLoc;
      cd->ValLoc(0) =           rslt;
      cd->ElemTyp(1) = A_Str;
      cd->Str(1) =            ".dword = D_Var + ((word *)r_dp - (word *)r_rp);";
      cd_add(cd);
      cd = alc_ary(3);
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) =              "VarLoc(";
      cd->ElemTyp(1) = A_ValLoc;
      cd->ValLoc(1) =           rslt;
      cd->ElemTyp(2) = A_Str;
      cd->Str(2) =              ") = (dptr)r_rp;";
      cd_add(cd);
      }

   cd = alc_ary(1);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =              "}";
   cd_add(cd);
   return rslt;
   }

/*
 * bound - bound the code for the given sub-tree. If catch_fail is true,
 *   direct failure to the bounding label.
 */
static struct val_loc *bound(n, rslt, catch_fail)
struct node *n;
struct val_loc *rslt;
int catch_fail;
   {
   struct code *lbl1;
   struct code *fail_sav;
   struct c_fnc *fnc_sav;

   fnc_sav = cur_fnc;
   fail_sav = on_failure;

   lbl1 = alc_lbl("bound", Bounding);
   cd_add(lbl1);
   cur_fnc->cursor = lbl1->prev;     /* code goes before label */
   if (catch_fail)
      on_failure = lbl1;

   rslt = gencode(n, rslt);

   cd_add(sig_cd(lbl1, cur_fnc));   /* transfer control to bounding label */
   cur_fnc = fnc_sav;
   cur_fnc->cursor = lbl1;

   on_failure = fail_sav;
   return rslt;
   }

/*
 * cd_add - add a code struct at the cursor in the current function.
 */
void cd_add(cd)
struct code *cd;
   {
   register struct code *cursor;

   cursor = cur_fnc->cursor;

   cd->next = cursor->next;
   cd->prev = cursor;
   if (cursor->next != NULL)
      cursor->next->prev = cd;
   cursor->next = cd;
   cur_fnc->cursor = cd;
   }

/*
 * sig_cd - convert a signal/label into a goto or return signal in
 *   the context of the given function.
 */
struct code *sig_cd(sig, fnc)
struct code *sig;
struct c_fnc *fnc;
   {
   struct code *cd;

   if (sig->cd_id == C_Label && sig->Container == fnc)
      return mk_goto(sig);
   else {
      cd = NewCode(1);      /* # fields <= # fields of C_Goto */
      cd->cd_id = C_RetSig;
      cd->next = NULL;
      cd->prev = NULL;
      cd->SigRef = add_sig(sig, fnc);
      return cd;
      }
   }

/*
 * add_sig - add signal to list of signals returned by function.
 */
struct sig_lst *add_sig(sig, fnc)
struct code *sig;
struct c_fnc *fnc;
   {
   struct sig_lst *sl;

   for (sl = fnc->sig_lst; sl != NULL && sl->sig != sig; sl = sl->next)
      ;
   if (sl == NULL) {
      sl = NewStruct(sig_lst);
      sl->sig = sig;
      sl->ref_cnt = 1;
      sl->next = fnc->sig_lst;
      fnc->sig_lst = sl;
      }
   else
      ++sl->ref_cnt;
   return sl;
   }

/*
 * callc_add - add code to call a continuation. Note the action to be
 *  taken if the continuation returns resumption. The actual list
 *  signals returned and actions to take will be figured out after
 *  the continuation has been optimized.
 */
void callc_add(cont)
struct c_fnc *cont;
   {
   struct code *cd;

   cd = new_call();
   cd->OperName = NULL;
   cd->Cont = cont;
   cd->ArgLst = NULL;
   cd->ContFail = on_failure;
   cd->SigActs = NULL;
   ++cont->ref_cnt;
   }

/*
 * callo_add - add code to call an operation.
 */
void callo_add(oper_nm, ret_flag, cont, need_cont, arglist, on_ret)
char *oper_nm;
int ret_flag;
struct c_fnc *cont;
int need_cont;
struct code *arglist;
struct code *on_ret;
   {
   struct code *cd;
   struct code *cd1;

   cd = new_call();
   cd->OperName = oper_nm;
   cd->Cont = cont;
   if (need_cont)
      cd->Flags = NeedCont;
   cd->ArgLst = arglist;
   cd->ContFail = NULL;   /* operation handles failure from the continuation */
   /*
    * Decide how to handle the signals produced by the operation. (Those
    *  produced by the continuation will be examined after the continuation
    *  is optimized.)
    */
   cd->SigActs = NULL;
   if (MightFail(ret_flag))
      cd->SigActs = new_sgact(&resume, sig_cd(on_failure,cur_fnc), cd->SigActs);
   if (ret_flag & DoesRet)
      cd->SigActs = new_sgact(&contin, on_ret, cd->SigActs);
   if (ret_flag & DoesFThru) {
      cd1 = NewCode(1);      /* #fields == #fields C_Goto */
      cd1->cd_id = C_Break;
      cd1->next = NULL;
      cd1->prev = NULL;
      cd->SigActs = new_sgact(&fallthru, cd1, cd->SigActs);
   }
   if (cont != NULL)
      ++cont->ref_cnt;  /* increment reference count */
}

/* 
 * Create a call, add it to the code for the current function, and 
 *  add it to the list of calls from the current function.
 */
static struct code *new_call()
   {
   struct code *cd;

   cd = NewCode(7);
   cd->cd_id = C_CallSig;
   cd_add(cd);
   cd->Flags = 0;
   cd->NextCall = cur_fnc->call_lst;
   cur_fnc->call_lst = cd;
   return cd;
   }

/*
 * sig_act - create a new binding of an action to a signal.
 */
struct sig_act *new_sgact(sig, cd, next)
struct code *sig;
struct code *cd;
struct sig_act *next;
   {
   struct sig_act *sa;

   sa = NewStruct(sig_act);
   sa->sig = sig;
   sa->cd = cd;
   sa->shar_act = NULL;
   sa->next = next;
   return sa;
   }


#ifdef OptimizeLit
static int instr(const char *str, int chr)  {
   int i, found, go;

   found = 0; go = 1;
   for(i=0; ((str[i] != '\0') && go) ;i++) {
      if (str[i] == chr) {
         go = 0;
         found = 1;
         if ((str[i+1] != '\0') && (chr == '='))
            if (str[i+1] == '=')
               found = 0;
         if ((chr == '=') && (i > 0))  {
            if (str[i-1] == '>')
               found = 0;
            else if (str[i-1] == '<')
               found = 0;
            else if (str[i-1] == '!')
               found = 0;
         }
      }
   }
   return found;
}

static void tbl_add(struct lit_tbl *add)  {
   struct lit_tbl *ins;
   static struct lit_tbl *ptr = NULL;
   int    go = 1;

   if (tbl == NULL) {
      tbl = add;
      ptr = add;
   }
   else  {
      ins = ptr;
      while ((ins != NULL) && go)  {
         if (add->index != ins->index)
            ins = ins->prev;
         else
            go = 0;
      }
      if (ins != NULL) {
         if (ins->end == NULL)
            ins->end = add->initial;
      }
      ptr->next = add;
      add->prev = ptr;
      ptr = add;
   }
}


static void invalidate(struct val_loc *val, struct code *end, int code)  {
   struct lit_tbl  *ptr, *back;
   int    index, go = 1;

   if (val == NULL)
      return;
   if (val->loc_type == V_NamedVar) {
      index = val->u.nvar->val.index;
      return;
   }
   else if (val->loc_type == V_Temp)
      index = val->u.tmp + cur_proc->tnd_loc;
   else
      return;
   if (tbl == NULL)
      return;
   back = tbl;
   while (back->next != NULL)
      back = back->next;
   go = 1;
   for(ptr=back; ((ptr != NULL) && go) ; ptr=ptr->prev) {
      if ((ptr->index == index) && (ptr->modified != NO_TOUCH)) {
         ptr->modified = code; 
         if ((code != LIMITED_TO_INT) && (ptr->safe)) {
            ptr->end = end;
            ptr->safe = 0;
         }
         go = 0;
      }
      else if ((ptr->index == index) && (ptr->modified == NO_TOUCH)) {
         if ((code != LIMITED_TO_INT) && (ptr->safe)) {
            ptr->end = end;
            ptr->safe = 0;
         }
         go = 0;
      }
      else if (ptr->index == index)
         go = 0;
   }
}


static int eval_code(struct code *cd, struct lit_tbl *cur)  {
   struct code *tmp;
   struct lit_tbl *tmp_tbl;
   int    i, j;
   char  *str;

   for (i=0; cd->ElemTyp(i) != A_End ;i++)  {
      switch(cd->ElemTyp(i))  {
         case A_ValLoc:
            if (cd->ValLoc(i)->mod_access != M_CInt)
               break;
            if ((cd->ValLoc(i)->u.tmp + cur_proc->tnd_loc) == cur->index) {
               switch (cd->ValLoc(i)->loc_type)  {
                  case V_Temp:
                     if (cur->csym->flag == F_StrLit)  {
#if 0
                        cd->ElemTyp(i) = A_Str;
                        str = (char *)alloc(strlen(cur->csym->image)+8);
			sprintf(str, "\"%s\"/*Z*/", cur->csym->image);
                        cd->Str(i) = str;
#endif
                     }
                     else if (cur->csym->flag == F_IntLit) {
                        cd->ElemTyp(i) = A_Str;
                        cd->Str(i) = cur->csym->image;
                     }
                     break;
                  default:
                     break;
               }
            }
            break;
         case A_Ary:
            for(tmp=cd->Array(i); tmp != NULL ;tmp=tmp->next)
               eval_code(tmp, cur);
            break;
         default:
            break;
      }
   }
}

static void propagate_literals()  {
   struct lit_tbl *ptr;
   struct code    *cd, *arg;
   int    ret;

   for(ptr=tbl; ptr != NULL ;ptr=ptr->next)  {
      if (ptr->modified != NO_TOUCH) {
         for(cd=ptr->initial; cd != ptr->end ;cd=cd->next)  {
            switch (cd->cd_id)  {
               case C_If:
                  for(arg=cd->Cond; arg != NULL ;arg=arg->next)
                     ret = eval_code(arg, ptr);
		  /*
		   * Again, don't take the 'then' portion.
		   * It might lead to infinite loops.
		   *  for(arg=cd->ThenStmt; arg != NULL ;arg=arg->next)
                   *      ret = eval_code(arg, ptr);
		   */
                  break; 
               case C_CdAry:
                  ret = eval_code(cd, ptr);
                  break; 
               case C_CallSig:
                  for(arg=cd->ArgLst; arg != NULL ;arg=arg->next)
                     ret = eval_code(arg, ptr);
                  break; 
               default:
                  break; 
            }
         } 
      }
   }
}

/*
 * analyze_literals - analyzes the generated code to replace
 *                      complex record dereferences with  C
 *                      literals.
 */
static void analyze_literals(struct code *start, struct code *top, int lvl)  {
   struct code *ptr, *tmp, *not_null;
   struct lit_tbl *new_tbl;
   struct lbl_tbl *new_lbl;
   struct val_loc *prev = NULL;
   int i, inc=0, addr=0, assgn=0, equal = 0;

   for (ptr = start; ptr != NULL ; ptr = ptr->next)  {
      if (!lvl)
         not_null = ptr;
      else
         not_null = top;
      switch (ptr->cd_id)  {
         case C_NamedVar:
            break;
         case C_CallSig:
            analyze_literals(ptr->ArgLst, not_null, lvl+1);
            break;
         case C_Goto:
            break;
         case C_Label:
            break;
         case C_Lit:
            new_tbl = alc_tbl();
            new_tbl->initial = ptr;
            new_tbl->vloc = ptr->Rslt;
            new_tbl->csym = ptr->Literal;
            switch (ptr->Rslt->loc_type) {
               case V_NamedVar:
                  new_tbl->index = ptr->Rslt->u.nvar->val.index;
                  tbl_add(new_tbl);
                  break;
               case V_Temp:
                  new_tbl->index = ptr->Rslt->u.tmp + cur_proc->tnd_loc;
                  tbl_add(new_tbl);
                  break;
               default:
                  new_tbl->index = -1;
                  free(new_tbl);
                  break;
            }
            break;
         case C_If:
            analyze_literals(ptr->Cond, not_null, lvl+1);
	    /*
	     *  Don't analyze the 'then' portion such as in:
             *   analyze_literals(ptr->ThenStmt, not_null, lvl+1);
	     *  Apparently, all the intermediate code does is maintain
	     *   a pointer to where the flow of execution jumps to in
	     *   case the 'then' is taken.  These are all goto statments
	     *   and can result in infinite loops of analyzation.
             */
            break;
         case C_CdAry:
            for(i=0; ptr->ElemTyp(i) != A_End ;i++)  {
               switch(ptr->ElemTyp(i))  {
                  case A_Str:
                     if (ptr->Str(i) != NULL) {
			if ( (strstr(ptr->Str(i), "-=")) ||
			     (strstr(ptr->Str(i), "+=")) ||
			     (strstr(ptr->Str(i), "*=")) ||
			     (strstr(ptr->Str(i), "/=")) )
                           invalidate(prev, not_null, NO_TOUCH);
                        else if (instr(ptr->Str(i), '=')) {
                           invalidate(prev, not_null, LIMITED);
                           assgn = 1;
                        }
                        else if ( (strstr(ptr->Str(i), "++")) ||
				  (strstr(ptr->Str(i), "--")) )
                           inc = 1;
                        else if (instr(ptr->Str(i), '&'))
                           addr = 1;
                        else if (strstr(ptr->Str(i), "=="))
                           equal = 1;
                     }
                     break;
                  case A_ValLoc:
                     if (inc) {
                        invalidate(ptr->ValLoc(i), not_null, NO_TOUCH);
                        inc = 0;
                     }
                     if (addr)  {
                        invalidate(ptr->ValLoc(i), not_null, LIMITED);
                        addr = 0;
                     }
                     if ((assgn) && (ptr->ValLoc(i)->mod_access == M_None)) {
                        invalidate(ptr->ValLoc(i), not_null, LIMITED);
                        assgn = 0;
                     }
                     else if (assgn)
                        assgn = 0;
                     if (equal) {
                        invalidate(ptr->ValLoc(i), not_null, LIMITED_TO_INT);
                        equal = 0;
                     }
                     prev = ptr->ValLoc(i);
                     break;
                  case A_Intgr:
                     break;
                  case A_SBuf:
                     break;
                  case A_Ary:
                     for(tmp=ptr->Array(i); tmp != NULL ;tmp=tmp->next)
                        analyze_literals(tmp, not_null, lvl+1);
                     break;
                  default:
                     break;
               }
            }
            break;
         default:
            break;
      }
   }
}
#endif					/* OptimizeLit */

/*
 * analyze_poll - analyzes the internal C code representation from
 *                the position of the last Poll() function call to 
 *                the current position in the code.
 *                Returns a 0 if the last Poll() function should not
 *                be removed.
 */
#ifdef OptimizePoll
static int analyze_poll(void)  {
   struct code *cursor, *ptr;
   int          cont = 1;

   ptr = lastpoll;
   if (ptr == NULL)
      return 0;
   cursor = cur_fnc->cursor; 
   while ((cursor != ptr) && (ptr != NULL) && (cont))  {
      switch (ptr->cd_id)  {
         case C_Null :
         case C_NamedVar :
         case C_Label    :
         case C_Lit      :
         case C_Resume   :
         case C_Continue :
         case C_FallThru :
         case C_PFail    :
         case C_Goto     :
         case C_Create   :
         case C_If       :
         case C_SrcLoc   :
         case C_CdAry    :
                 break;
         case C_CallSig  :
         case C_RetSig   :
         case C_LBrack   :
         case C_RBrack   :
         case C_PRet     :
         case C_PSusp    :
         case C_Break    :
                 cont = 0;
                 break;
      }
      ptr = ptr->next;
   }
   return cont;
}

/*
 * remove_poll - removes the ccode structure that represents the last
 *  call to the "Poll()" function by simply changing the code ID to
 *  C_Null code.
 */
static void remove_poll(void)  {
   
   if (lastpoll == NULL)
      return;
   lastpoll->cd_id = C_Null;
}
#endif					/* OptimizePoll */

/*
 * setloc produces code to set the file name and line number to the
 *  source location of node n.  Code is only produced if the corresponding
 *  value has changed since the last time setloc was called.
 */
static void setloc(n)
nodeptr n;
   {
   struct code *cd;
   static int   count=0;

   if (n == NULL || File(n) == NULL || Line(n) == 0)
      return;

   if (File(n) != lastfiln || Line(n) != lastline) {
#ifdef OptimizePoll
      if (analyze_poll())
         remove_poll();
      cd = alc_ary(1);
      lastpoll = cd;
#else					/* OptimizePoll */
      cd = alc_ary(1);
#endif					/* OptimizePoll */
      cd->ElemTyp(0) = A_Str;
      cd->Str(0) = "Poll();";
      cd_add(cd);
      
      if (line_info) {
         cd = NewCode(2);
         cd->cd_id = C_SrcLoc;
   
         if (File(n) == lastfiln)
            cd->FileName = NULL;
         else {
            lastfiln = File(n);
            cd->FileName = lastfiln;
            }
   
         if (Line(n) == lastline)
            cd->LineNum = 0;
         else {
            lastline = Line(n);
            cd->LineNum = lastline;
            }
   
         cd_add(cd);
         }
      }
   }

/*
 * alc_ary - create an array for a sequence of code fragments.
 */
struct code *alc_ary(n)
int n;
   {
   struct code *cd;
   static cnt=1;

   cd = NewCode(2 * n + 1);
   cd->cd_id = C_CdAry;
   cd->next = NULL;
   cd->prev = NULL;
   cd->ElemTyp(n) = A_End;
   return cd;
   }


/*
 * alc_lbl - create a label.
 */
struct code *alc_lbl(desc, flag)
char *desc;
int flag;
   {
   register struct code *cd;

   cd = NewCode(5);
   cd->cd_id = C_Label;
   cd->next = NULL;
   cd->prev = NULL;
   cd->Container = cur_fnc; /* function containing label */
   cd->SeqNum = 0;          /* sequence number is allocated later */
   cd->Desc = desc;         /* identifying comment */
   cd->RefCnt = 0;          /* reference count */
   cd->LabFlg = flag;
   return cd;
   }

/*
 * alc_fnc - allocate a function structure;
 */
static struct c_fnc *alc_fnc()
   {
   register struct c_fnc *cf;
   int i;

   cf = NewStruct(c_fnc);
   cf->prefix[0] = '\0';             /* prefix is allocated later */
   cf->prefix[PrfxSz] = '\0';        /* terminate prefix for printing */
   cf->flag = 0;
   for (i = 0; i < PrfxSz; ++i)
      cf->frm_prfx[i] = frm_prfx[i]; /* note procedure frame prefix */
   cf->frm_prfx[PrfxSz] = '\0';      /* terminate prefix for printing */
   cf->cd.cd_id = C_Null;            /* base of code sequence in function */
   cf->cd.next = NULL;
   cf->cursor = &cf->cd;             /* current place to insert code */
   cf->call_lst = NULL;              /* functions called by this function */
   cf->creatlst = NULL;              /* creates within this function */
   cf->sig_lst = NULL;               /* signals returned by this function */
   cf->ref_cnt = 0;
   cf->next = NULL;
   *flst_end = cf;                   /* link entry onto global list */
   flst_end = &(cf->next);
   return cf;
   }

/*
 * tmp_loc - allocate a value location structure for nth temporary descriptor
 *  variable in procedure frame.
 */
static struct val_loc *tmp_loc(n)
int n;
   {
   register struct val_loc *r;

   r = NewStruct(val_loc);
   r->loc_type = V_Temp;
   r->mod_access = M_None;
   r->u.tmp = n;
   return r;
   }

/*
 * itmp_loc - allocate a value location structure for nth temporary integer
 *  variable in procedure frame.
 */
struct val_loc *itmp_loc(n)
int n;
   {
   register struct val_loc *r;

   r = NewStruct(val_loc);
   r->loc_type = V_ITemp;
   r->mod_access = M_None;
   r->u.tmp = n;
   return r;
   }

/*
 * dtmp_loc - allocate a value location structure for nth temporary double
 *  variable in procedure frame.
 */
struct val_loc *dtmp_loc(n)
int n;
   {
   register struct val_loc *r;

   r = NewStruct(val_loc);
   r->loc_type = V_DTemp;
   r->mod_access = M_None;
   r->u.tmp = n;
   return r;
   }

/*
 * vararg_sz - allocate a value location structure that refers to the size
 *  of the variable part of an argument list.
 */
static struct val_loc *vararg_sz(n)
int n;
   {
   register struct val_loc *r;

   r = NewStruct(val_loc);
   r->loc_type = V_Const;
   r->mod_access = M_None;
   r->u.int_const = n;
   return r;
   }

/*
 * cvar_loc - allocate a value location structure for a C variable.
 */
struct val_loc *cvar_loc(name)
char *name;
   {
   register struct val_loc *r;

   r = NewStruct(val_loc);
   r->loc_type = V_CVar;
   r->mod_access = M_None;
   r->u.name = name;
   return r;
   }

/*
 * var_ref - allocate a value location structure for an Icon named variable.
 */
static struct val_loc *var_ref(sym)
struct lentry *sym;
   {
   struct val_loc *loc;

   loc = NewStruct(val_loc);
   loc->loc_type = V_NamedVar;
   loc->mod_access = M_None;
   loc->u.nvar = sym;
   return loc;
   }

/*
 * deref_cd - generate code to dereference a descriptor.
 */
static void deref_cd(src, dest)
struct val_loc *src;
struct val_loc *dest;
   {
   struct code *cd;

   cd = alc_ary(5);
   cd->ElemTyp(0) = A_Str;
   cd->Str(0) =                  "deref(&";
   cd->ElemTyp(1) = A_ValLoc;
   cd->ValLoc(1) =               src;
   cd->ElemTyp(2) = A_Str;
   cd->Str(2) =                  ", &";
   cd->ElemTyp(3) = A_ValLoc;
   cd->ValLoc(3) =               dest;
   cd->ElemTyp(4) = A_Str;
   cd->Str(4) =                  ");";
   cd_add(cd);
   }

/*
 * inv_op - directly invoke a run-time operation, in-lining it if possible.
 */
static struct val_loc *inv_op(n, rslt)
nodeptr n;
struct val_loc *rslt;
   {
   struct implement *impl;
   struct code *scont_strt;
   struct code *scont_fail;
   struct c_fnc *fnc;
   struct val_loc *frst_arg;
   struct val_loc *arg_rslt;
   struct val_loc *r;
   struct val_loc **varg_rslt;
   struct op_symentry *symtab;
   struct lentry **single;
   struct tmplftm *lifetm_ary;
   nodeptr rslt_lftm;
   char *sbuf;
   int *maybe_var;
   int may_mod;
   int nsyms;
   int nargs;
   int nparms;
   int cont_loc;
   int flag;
   int refs;
   int var_args;
   int n_varargs;
   int arg_loc;
   int dcl_var;
   int i;
   int j;
   int v;

   nargs = Val0(n);
   impl = Impl1(n);
   if (impl == NULL) {
      /*
       * We have already printed an error, just make sure we can
       *  continue.
       */
      return &ignore;
      }

   /*
    * If this operation uses its result location as a work area, it must
    *   be given a tended result location and the value must be retained
    *   as long as the operation can be resumed.
    */
   rslt_lftm = n->lifetime;
   if (impl->use_rslt) {
       rslt_lftm = max_lftm(rslt_lftm, n->intrnl_lftm);
       if (rslt == &ignore)
          rslt = NULL;      /* force allocation of temporary */
       }

   /*
    * Determine if this operation takes a variable number of arguments
    *  and determine the size of the variable part of the arg list.
    */
   nparms = impl->nargs;
   if (nparms > 0 && impl->arg_flgs[nparms - 1] & VarPrm) {
      var_args = 1;
      n_varargs = nargs - nparms + 1;
      if (n_varargs < 0)
         n_varargs = 0;
      }
   else {
      var_args = 0;
      n_varargs = 0;
      }

   /*
    * Construct a symbol table (implemented as an array) for the operation.
    *  The symbol table includes parameters, and both the tended and
    *  ordinary variables from the RTL declare statement.
    */
   nsyms = (n->symtyps == NULL ? 0 : n->symtyps->nsyms);
   if (var_args)
      ++nsyms;
   nsyms += impl->ntnds + impl->nvars;
   if (nsyms > 0)
      symtab = (struct op_symentry *)alloc((unsigned int)(nsyms *
         sizeof(struct op_symentry)));
   else
      symtab = NULL;
   for (i = 0; i < nsyms; ++i) {
      symtab[i].n_refs = 0;     /* number of non-modifying references */
      symtab[i].n_mods = 0;     /* number of modifying references */
      symtab[i].n_rets = 0;     /* number of times returned directly */
      symtab[i].var_safe = 0;   /* Icon variable arg can be passed directly */
      symtab[i].adjust = 0;     /* adjustments needed to "dereference" */
      symtab[i].itmp_indx = -1; /* loc after "in-place" convert to C integer */
      symtab[i].dtmp_indx = -1; /* loc after "in-place" convert to C double */
      symtab[i].loc = NULL;     /* location as a descriptor */
      }

   /*
    * If in-lining has not been disabled or the operation is a keyword,
    *  check to see if it can reasonably be in-lined and gather information
    *  needed to in-line it.
    */
   if ((allow_inline || impl->oper_typ == 'K') &&
      do_inlin(impl, n, &cont_loc, symtab, n_varargs)) {
      /*
       * In-line the operation.
       */

      if (impl->ret_flag & DoesRet || impl->ret_flag & DoesSusp)
         rslt = chk_alc(rslt, rslt_lftm);  /* operation produces a result */

      /*
       * Allocate arrays to hold information from type inferencing about
       *  whether arguments are variables. This is used to optimize
       *  dereferencing.
       */
      if (nargs > 0) {
         maybe_var = (int *)alloc((unsigned int)(nargs * sizeof(int)));
         single = (struct lentry **)alloc((unsigned int)(nargs *
            sizeof(struct lentry *)));
         }

      if (var_args)
         --nparms; /* don't deal with varargs parameter yet. */

      /*
       * Match arguments with parameters and generate code for the
       *  arguments. The type of code generated depends on the kinds
       *  of dereferencing optimizations that are possible, though
       *  in general, dereferencing must wait until all arguments are
       *  computed. Because there may be both dereferenced and undereferenced
       *  parameters for an argument, the symbol table index does not always
       *  match the argument index.
       */
      i = 0;  /* symbol table index */
      for (j = 0; j < nparms && j < nargs; ++j) {
         /*
          * Use information from type inferencing to determine if the
          *  argument might me a variable and whether it is a single
          *  known named variable.
          */
         maybe_var[j] = HasVar(varsubtyp(n->n_field[FrstArg + j].n_ptr->type,
             &(single[j])));

         /*
          * Determine how many times the argument is referenced. If we
          *  optimize away return statements because we don't need the
          *  result, those references don't count. Take into account
          *  that there may be both dereferenced and undereferenced
          *  parameters for this argument.
          */
         if (rslt == &ignore)
            symtab[i].n_refs -= symtab[i].n_rets;
         refs = symtab[i].n_refs + symtab[i].n_mods;
         flag = impl->arg_flgs[j] & (RtParm | DrfPrm);
         if (flag == (RtParm | DrfPrm))
            refs += symtab[i + 1].n_refs + symtab[i + 1].n_mods;
         if (refs == 0) {
            /*
             * Indicate that we don't need the argument value (we must
             *  still perform the computation in case it has side effects).
             */
            arg_rslt = &ignore;
            symtab[i].adjust = AdjNone;
            }
         else {
            /*
             * Decide whether the result location for the argument can be
             *  used directly as the parameter.
             */
            if (flag == (RtParm | DrfPrm) && symtab[i].n_refs +
               symtab[i].n_mods == 0) {
                  /*
                   * We have both dereferenced and undereferenced parameters,
                   *  but don't use the undereferenced one so ignore it.
                   */
                  symtab[i].adjust = AdjNone;
                  ++i;
                  flag = DrfPrm;
                  }    
            if (flag == DrfPrm && single[j] != NULL) {
               /*
                * We need only a dereferenced value, but know what variable
                *  it is in. We don't need the computed argument value, we will
                *  get it directly from the variable. If it is safe to do
                *  so, we will pass a pointer to the variable as the argument
                *  to the operation.
                */
               arg_rslt = &ignore;
               symtab[i].loc = var_ref(single[j]);
               if (symtab[i].var_safe)
                  symtab[i].adjust = AdjNone;
               else
                  symtab[i].adjust = AdjCpy;
               }
            else {
               /*
                * Determine if the argument descriptor is modified by the
                *  operation; dereferencing a variable is a modification.
                */
               may_mod = (symtab[i].n_mods != 0);
               if (flag == DrfPrm)
                  may_mod |= maybe_var[j];
               if (n->n_field[FrstArg + j].n_ptr->reuse && may_mod) {
                  /*
                   * The parameter may be reused without recomputing
                   *  the argument and the value may be modified. The
                   *  argument result location and the parameter location
                   *  must be separate so the parameter is reloaded upon
                   *  each invocation.
                   */
                  arg_rslt = chk_alc(NULL,
                     n->n_field[FrstArg + j].n_ptr->lifetime);
                  if (flag == DrfPrm && maybe_var[j])
                     symtab[i].adjust = AdjNDrf;  /* var: must dereference */
                  else
                     symtab[i].adjust = AdjCpy;   /* value only: just copy */
                  }
               else {
                  /*
                   * Argument result location will act as parameter location.
                   *  Its lifetime must be as long as both that of the
                   *  the argument and the parameter (operation internal
                   *  lifetime).
                   */
                  arg_rslt = chk_alc(NULL, max_lftm(n->intrnl_lftm,
                     n->n_field[FrstArg + j].n_ptr->lifetime));
                  if (flag == DrfPrm && maybe_var[j])
                     symtab[i].adjust = AdjDrf;   /* var: must dereference */
                  else
                     symtab[i].adjust = AdjNone;
                  }
               symtab[i].loc = arg_rslt;
               }
            }

         /*
          * Generate the code for the argument.
          */
         gencode(n->n_field[FrstArg + j].n_ptr, arg_rslt);

         if (flag == (RtParm | DrfPrm)) {
            /*
             * We have computed the value for the undereferenced parameter,
             *  decide how to get the dereferenced value.
             */
            ++i;
            if (symtab[i].n_refs + symtab[i].n_mods == 0)
               symtab[i].adjust = AdjNone;  /* not needed, ignore */
            else {
               if (single[j] != NULL) {
                  /*
                   * The value is in a specific Icon variable, get it from
                   *  there. If is is safe to pass the variable directly
                   *  to the operation, do so.
                   */
                  symtab[i].loc = var_ref(single[j]);
                  if (symtab[i].var_safe)
                     symtab[i].adjust = AdjNone;
                  else
                     symtab[i].adjust = AdjCpy;
                  }
               else {
                  /*
                   * If there might be a variable reference, note that it
                   *  must be dereferenced. Otherwise decide whether the
                   *  argument location can be used for both the dereferenced
                   *  and undereferenced parameter.
                   */
                  symtab[i].loc = arg_rslt;
                  if (maybe_var[j])
                     symtab[i].adjust = AdjNDrf;
                  else if (symtab[i - 1].n_mods + symtab[i].n_mods == 0)
                     symtab[i].adjust = AdjNone;
                  else
                     symtab[i].adjust = AdjCpy;
                  }
               }
            }
         ++i;
         }

      /*
       * Fill out parameter list with null values.
       */
      while (j < nparms) {
         int k, kn;
         kn = 0;
         if (impl->arg_flgs[j] & RtParm)
            ++kn;
         if (impl->arg_flgs[j] & DrfPrm)
            ++kn;
         for (k = 0; k < kn; ++k) {
            if (symtab[i].n_refs + symtab[i].n_mods > 0) {
               arg_rslt = chk_alc(NULL, n->intrnl_lftm);
               cd_add(asgn_null(arg_rslt));
               symtab[i].loc = arg_rslt;
               }
            symtab[i].adjust = AdjNone;
            ++i;
            }
         ++j;
         }

      if (var_args) {
         /*
          * Compute variable part of argument list.
          */
         ++nparms;   /* add varargs parameter back into parameter list  */

         /*
          * The variable part of the parameter list must be in contiguous
          *  descriptors. Create location and lifetime arrays for use in
          *  allocating the descriptors.
          */
         if (n_varargs > 0) {
            varg_rslt = (struct val_loc **)alloc((unsigned int)(n_varargs *
               sizeof(struct val_loc *)));
            lifetm_ary = alc_lftm(n_varargs, NULL);
            }

         flag = impl->arg_flgs[j] & (RtParm | DrfPrm);

         /*
          * Compute the lifetime of the elements of the varargs parameter array.
          */
         for (v = 0; v < n_varargs; ++v) {
            /*
             * Use information from type inferencing to determine if the
             *  argument might me a variable and whether it is a single
             *  known named variable.
             */
            maybe_var[j + v] = HasVar(varsubtyp(
               n->n_field[FrstArg+j+v].n_ptr->type, &(single[j + v])));

            /*
             * Determine if the elements of the vararg parameter array
             *  might be modified. If it is a variable, dereferencing
             *  modifies it.
             */
            may_mod = (symtab[j].n_mods != 0);
            if (flag == DrfPrm)
               may_mod |= maybe_var[j + v];

            if ((flag == DrfPrm && single[j + v] != NULL) ||
               (n->n_field[FrstArg + j + v].n_ptr->reuse && may_mod)) {
               /*
                * The argument value is only placed in the vararg parameter
                *  array during "dereferencing". So the lifetime of the array
                *  element is the lifetime of the parameter and the element
                *  is not used until dereferencing.
                */
               lifetm_ary[v].lifetime = n->intrnl_lftm;
               lifetm_ary[v].cur_status = n->postn;
               }
            else {
               /*
                * The argument is computed into the vararg parameter array.
                *  The lifetime of the array element encompasses both
                *  the lifetime of the argument and the parameter. The
                *  element is used as soon as the argument is computed.
                */
               lifetm_ary[v].lifetime = max_lftm(n->intrnl_lftm,
                   n->n_field[FrstArg+j+v].n_ptr->lifetime);
               lifetm_ary[v].cur_status = n->n_field[FrstArg+j+v].n_ptr->postn;
               }
            }

         /*
          * Allocate (reserve) the array of temporary variables for the
          *  vararg list.
          */
         if (n_varargs > 0) {
            arg_loc = alc_tmp(n_varargs, lifetm_ary);
            free((char *)lifetm_ary);
            }

         /*
          * Generate code to compute arguments.
          */
         for (v = 0; v < n_varargs; ++v) {
            may_mod = (symtab[j].n_mods != 0);
            if (flag == DrfPrm)
               may_mod |= maybe_var[j + v];
            if (flag == DrfPrm && single[j + v] != NULL) {
              /*
               * We need a dereferenced value and it is in a known place: a
               *  named variable; don't bother saving the result of the
               *  argument computation.
               */
               r = &ignore;
               }
            else if (n->n_field[FrstArg + j + v].n_ptr->reuse && may_mod) {
               /* 
                * The argument can be reused without being recomputed and
                *  the parameter may be modified, so we cannot safely
                *  compute the argument into the vararg parameter array; we
                *  must compute it elsewhere and copy (dereference) it at the
                *  beginning of the operation. Let gencode allocate an argument
                *  result location.
                */
               r = NULL; 
               }
            else {
               /*
                * We can compute the argument directly into the vararg
                *  parameter array.
                */
               r = tmp_loc(arg_loc + v);
               }
            varg_rslt[v] = gencode(n->n_field[FrstArg + j + v].n_ptr, r);
            }

         setloc(n);
         /*
          * Dereference or copy argument values that are not already in vararg
          *  parameter list. Preceding arguments are dereferenced later, but
          *  it is okay if dereferencing is out-of-order.
          */
         for (v = 0; v < n_varargs; ++v) {
            if (flag == DrfPrm && single[j + v] != NULL) {
               /*
                * Copy the value from the known named variable into the
                *  parameter list.
                */
               varg_rslt[v] = var_ref(single[j + v]);
               cd_add(mk_cpyval(tmp_loc(arg_loc + v), varg_rslt[v]));
               }
            else if (flag == DrfPrm && maybe_var[j + v]) {
               /*
                * Dereference the argument into the parameter list.
                */
               deref_cd(varg_rslt[v], tmp_loc(arg_loc + v));
               }
            else if (arg_loc + v != varg_rslt[v]->u.tmp) {
               /*
                * The argument is a dereferenced value, but is not yet
                *  in the parameter list; copy it there.
                */
               cd_add(mk_cpyval(tmp_loc(arg_loc + v), varg_rslt[v]));
               }
            tmp_status[arg_loc + v] = InUse; /* parameter location in use */
            }

         /*
          * The vararg parameter gets the address of the first element
          *  in the variable part of the argument list and the size
          *  parameter gets the number of elements in the list.
          */
         if (n_varargs > 0) {
            free((char *)varg_rslt);
            symtab[i].loc = tmp_loc(arg_loc);
            }
         else
            symtab[i].loc = chk_alc(NULL, n); /* dummy arg location */
         symtab[i].loc->mod_access = M_Addr;
         ++i;
         symtab[i].loc = vararg_sz(n_varargs);
         ++i;
         }
      else {
         /*
          * Compute extra arguments, but discard the results.
          */
         while (j < nargs) {
            gencode(n->n_field[FrstArg + j].n_ptr, &ignore);
            ++j;
            }
         }

      if (nargs > 0) {
         free((char *)maybe_var);
         free((char *)single);
         }

      /*
       * If execution does not continue through the parameter evaluation,
       *  don't try to generate in-line code. A lack of parameter types
       *  will cause problems with some in-line type conversions.
       */
      if (!past_prms(n))
         return rslt;

      setloc(n);

      dcl_var = i;

      /*
       * Perform any needed copying or dereferencing.
       */
      for (i = 0; i < nsyms; ++i) {
         switch (symtab[i].adjust) {
            case AdjNDrf:
               /*
                * Dereference into a new temporary which is used as the
                *  parameter.
                */
               arg_rslt = chk_alc(NULL, n->intrnl_lftm);
               deref_cd(symtab[i].loc, arg_rslt);
               symtab[i].loc = arg_rslt;
               break;
            case AdjDrf:
               /*
                * Dereference in place.
                */
               deref_cd(symtab[i].loc, symtab[i].loc);
               break;
           case AdjCpy:
               /*
                * Copy into a new temporary which is used as the
                *  parameter.
                */
              arg_rslt = chk_alc(NULL, n->intrnl_lftm);
              cd_add(mk_cpyval(arg_rslt, symtab[i].loc));
              symtab[i].loc = arg_rslt;
              break;
           case AdjNone:
              break;     /* nothing need be done */
            }
         }

      switch (cont_loc) {
         case SepFnc:
            /*
             * success continuation must be in a separate function.
             */
            fnc = alc_fnc();
            sbuf = (char *)alloc((unsigned int)(strlen(impl->name) + 5));
            sprintf(sbuf, "end %s", impl->name);
            scont_strt = alc_lbl(sbuf, 0);
            cd_add(scont_strt);
            cur_fnc->cursor = scont_strt->prev; /* put oper before label */
            gen_inlin(impl->in_line, rslt, &scont_strt, NULL, fnc, impl,
               nsyms, symtab, n, dcl_var, n_varargs);
            cur_fnc->cursor = scont_strt;
            callc_add(fnc);
            cur_fnc = fnc;
            on_failure = &resume;
            break;
         case SContIL:
            /*
             * one suspend an no return: success continuation is put in-line.
             */
            gen_inlin(impl->in_line, rslt, &scont_strt, &scont_fail, NULL, impl,
                nsyms, symtab, n, dcl_var, n_varargs);
            cur_fnc->cursor = scont_strt;
            on_failure = scont_fail;
            break;
         case EndOper:
            /*
             * no suspends: success continuation goes at end of operation.
             */

            sbuf = (char *)alloc((unsigned int)(strlen(impl->name) + 5));
            sprintf(sbuf, "end %s", impl->name);
            scont_strt = alc_lbl(sbuf, 0);
            cd_add(scont_strt);
            cur_fnc->cursor = scont_strt->prev; /* put operation before lbl */
            gen_inlin(impl->in_line, rslt, &scont_strt, NULL, NULL, impl,
               nsyms, symtab, n, dcl_var, n_varargs);
            cur_fnc->cursor = scont_strt;
            break;
         }
      }
   else {
      /*
       * Do not in-line operation.
       */
      implproto(impl);
      frst_arg = gen_args(n, 2, nargs);
      setloc(n);
      if (impl->ret_flag & (DoesRet | DoesSusp))
         rslt = chk_alc(rslt, rslt_lftm);
      mk_callop(oper_name(impl), impl->ret_flag, frst_arg, nargs, rslt,
         0);
      }
   if (symtab != NULL)
      free((char *)symtab);
   return rslt;
   }

/*
 * max_lftm - given two lifetimes (in the form of nodes) return the
 *   maximum one.
 */
static nodeptr max_lftm(n1, n2)
nodeptr n1;
nodeptr n2;
   {
   if (n1 == NULL)
      return n2;
   else if (n2 == NULL)
      return n1;
   else if (n1->postn > n2->postn)
      return n1;
   else
      return n2;
   }

/*
 * inv_prc - directly invoke a procedure.
 */
static struct val_loc *inv_prc(n, rslt)
nodeptr n;
struct val_loc *rslt;
   {
   struct pentry *proc;
   struct val_loc *r;
   struct val_loc *arg1rslt;
   struct val_loc *var_part;
   int *must_deref;
   struct lentry **single;
   struct val_loc **arg_rslt;
   struct code *cd;
   struct tmplftm *lifetm_ary;
   char *sbuf;
   int nargs;
   int nparms;
   int i, j;
   int arg_loc;
   int var_sz;
   int var_loc;

   /*
    * This procedure is implemented without argument list adjustment or 
    *  dereferencing, so they must be done before the call.
    */
   nargs = Val0(n);              /* number of arguments */
   proc = Proc1(n);
   nparms = Abs(proc->nargs);

   if (nparms > 0) {
      must_deref = (int *)alloc((unsigned int)(nparms * sizeof(int)));
      single = (struct lentry **)alloc((unsigned int)(nparms *
         sizeof(struct lentry *)));
      arg_rslt = (struct val_loc **)alloc((unsigned int)(nparms *
         sizeof(struct val_loc *)));
      }

   /*
    * Allocate a work area of temporaries to use as argument list. If
    *  an argument can be reused without being recomputed, it must not
    *  be computed directly into the work area. It will be copied or
    *  dereferenced into the work area when execution reaches the
    *  operation. If an argument is a single named variable, it can
    *  be dereferenced directly into the argument location. These
    *  conditions affect when the temporary will receive a value.
    */
   if (nparms > 0)
      lifetm_ary = alc_lftm(nparms, NULL);
   for (i = 0; i < nparms; ++i)
      lifetm_ary[i].lifetime = n->intrnl_lftm;
   for (i = 0; i < nparms && i < nargs; ++i) {
      must_deref[i] = HasVar(varsubtyp(n->n_field[FrstArg + i].n_ptr->type,
         &(single[i])));
      if (single[i] != NULL || n->n_field[FrstArg + i].n_ptr->reuse)
         lifetm_ary[i].cur_status = n->postn;
      else
         lifetm_ary[i].cur_status = n->n_field[FrstArg + i].n_ptr->postn;
      }
   while (i < nparms) {
      lifetm_ary[i].cur_status = n->postn; /* arg list extension */
      ++i;
      }
   if (proc->nargs < 0)
      lifetm_ary[nparms - 1].cur_status = n->postn;  /* variable part */

   if (nparms > 0) {
      arg_loc = alc_tmp(nparms, lifetm_ary);
      free((char *)lifetm_ary);
      }
   if (proc->nargs < 0)
      --nparms;     /* treat variable part specially */
   for (i = 0; i < nparms && i < nargs;  ++i) {
      if (single[i] != NULL)
         r = &ignore;   /* we know where the dereferenced value is */
      else if (n->n_field[FrstArg + i].n_ptr->reuse)
         r = NULL;      /* let gencode allocate a new temporary */
      else
         r = tmp_loc(arg_loc + i);
      arg_rslt[i] = gencode(n->n_field[FrstArg + i].n_ptr, r);
      }

   /*
    * If necessary, fill out argument list with nulls.
    */
   while (i < nparms) {
      cd_add(asgn_null(tmp_loc(arg_loc + i)));
      tmp_status[arg_loc + i] = InUse;
      ++i;
      }

   if (proc->nargs < 0) {
      /*
       * handle variable part of list.
       */
      var_sz = nargs - nparms;

      if (var_sz > 0) {
         lifetm_ary = alc_lftm(var_sz, &n->n_field[FrstArg + nparms]);
         var_loc = alc_tmp(var_sz, lifetm_ary);
         free((char *)lifetm_ary);
         for (j = 0; j < var_sz; ++j) {
            gencode(n->n_field[FrstArg + nparms + j].n_ptr,
               tmp_loc(var_loc + j));
         }
      }
   }
   else {
      /*
       * If there are extra arguments, compute them, but discard the
       *  results.
       */
      while (i < nargs) {
         gencode(n->n_field[FrstArg + i].n_ptr, &ignore);
         ++i;
         }
      }

   setloc(n);
   /*
    * Dereference or copy argument values that are not already in argument
    *  list as dereferenced values.
    */
   for (i = 0; i < nparms && i < nargs;  ++i) {
      if (must_deref[i]) {
         if (single[i] == NULL)  {
            deref_cd(arg_rslt[i], tmp_loc(arg_loc + i));
         }
         else {
            arg_rslt[i] = var_ref(single[i]);
            cd_add(mk_cpyval(tmp_loc(arg_loc + i), arg_rslt[i]));
            }
         }
      else if (n->n_field[FrstArg + i].n_ptr->reuse)
         cd_add(mk_cpyval(tmp_loc(arg_loc + i), arg_rslt[i]));
      tmp_status[arg_loc + i] = InUse;
      }

   if (proc->nargs < 0) {
      var_part = tmp_loc(arg_loc + nparms);
      tmp_status[arg_loc + nparms] = InUse;
      if (var_sz <= 0) {
         cd = alc_ary(3);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =                 "varargs(NULL, 0, &";
         cd->ElemTyp(1) = A_ValLoc;
         cd->ValLoc(1) =               var_part;
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =                 ");";
         }
      else {
         cd = alc_ary(7);
         cd->ElemTyp(0) = A_Str;
         cd->Str(0) =                 "varargs(&";
         cd->ElemTyp(1) = A_ValLoc;
         cd->ValLoc(1) =               tmp_loc(var_loc);
         cd->ElemTyp(2) = A_Str;
         cd->Str(2) =                  ", ";
         cd->ElemTyp(3) = A_Intgr;
         cd->Intgr(3) =               var_sz; 
         cd->ElemTyp(4) = A_Str;
         cd->Str(4) =                 ", &";
         cd->ElemTyp(5) = A_ValLoc;
         cd->ValLoc(5) =               var_part;
         cd->ElemTyp(6) = A_Str;
         cd->Str(6) =                 ");";
         }
      cd_add(cd);
      ++nparms;   /* include variable part in call */
      }

   if (nparms > 0) {
      free((char *)must_deref);
      free((char *)single);
      free((char *)arg_rslt);
      }

   sbuf = (char *)alloc((unsigned int)(strlen(proc->name) + PrfxSz + 3));
   sprintf(sbuf, "P%s_%s", proc->prefix, proc->name);
   if (nparms > 0)
      arg1rslt = tmp_loc(arg_loc);
   else
      arg1rslt = NULL;
   if (proc->ret_flag & (DoesRet | DoesSusp))
      rslt = chk_alc(rslt, n->lifetime);
   mk_callop(sbuf, proc->ret_flag, arg1rslt, nargs, rslt, 1);
   return rslt;
   }

/*
 * endlife - link a temporary variable onto the list to be freed when
 *  execution reaches a node.
 */
static void endlife(kind, indx, old, n)
int kind;
int indx;
int old;
nodeptr n;
   {
   struct freetmp *freetmp;

   if ((freetmp = freetmp_pool) == NULL)
     freetmp = NewStruct(freetmp);
   else
      freetmp_pool = freetmp_pool->next;
   freetmp->kind = kind;
   freetmp->indx = indx;
   freetmp->old = old;
   freetmp->next = n->freetmp;
   n->freetmp = freetmp;
   }

/*
 * alc_tmp - allocate a block of temporary variables with the given lifetimes.
 */
static int alc_tmp(num, lifetm_ary)
int num;
struct tmplftm *lifetm_ary;
   {
   int i, j, k;
   register int status;
   int *new_status;
   int new_size;

   i = 0;
   for (;;) {
      if (i + num > status_sz) {
         /*
          * The status array is too small, expand it.
          */
         new_size = status_sz + Max(num, status_sz);
         new_status = (int *)alloc((unsigned int)(new_size * sizeof(int)));
         k = 0;
         while (k < status_sz) {
            new_status[k] = tmp_status[k];
            ++k;
            }
         while (k < new_size) {
            new_status[k] = NotAlloc;
            ++k;
            }
         free((char *)tmp_status);
         tmp_status = new_status;
         status_sz = new_size;
         }
      for (j = 0; j < num; ++j) {
         status = tmp_status[i + j];
         if (status != NotAlloc &&
            (status == InUse || status <= lifetm_ary[j].lifetime->postn))
               break;
         }
      /*
       * Did we find a block of temporaries that we can use?
       */
      if (j == num) {
         while (--j >= 0) {
            endlife(DescTmp, i + j, tmp_status[i + j], lifetm_ary[j].lifetime);
            tmp_status[i + j] = lifetm_ary[j].cur_status;
            }
         if (i + num > num_tmp)
            num_tmp = i + num;
         return i;
         }
      ++i;
      }
   }

/*
 * alc_lftm - allocate an array of lifetime information for an argument
 *  list.
 */
static struct tmplftm *alc_lftm(num, args)
int num;
union field *args;
   {
   struct tmplftm *lifetm_ary;
   int i;

   lifetm_ary = (struct tmplftm *)alloc((unsigned int)(num *
      sizeof(struct tmplftm)));
   if (args != NULL)
     for (i = 0; i < num; ++i) {
        lifetm_ary[i].cur_status = args[i].n_ptr->postn; /* reserved for arg */
        lifetm_ary[i].lifetime = args[i].n_ptr->lifetime;
        }
   return lifetm_ary;
   }

/*
 * alc_itmp - allocate a temporary C integer variable.
 */
int alc_itmp(lifetime)
nodeptr lifetime;
   {
   int i, j;
   int new_size;

   i = 0;
   while (i < istatus_sz && itmp_status[i] == InUse)
      ++i;
   if (i >= istatus_sz) {
      /*
       * The status array is too small, expand it.
       */
      free((char *)itmp_status);
      new_size = istatus_sz * 2;
      itmp_status = (int *)alloc((unsigned int)(new_size * sizeof(int)));
      j = 0;
      while (j < istatus_sz)
         itmp_status[j++] = InUse;
      while (j < new_size)
         itmp_status[j++] = NotAlloc;
      istatus_sz = new_size;
      }
   endlife(CIntTmp, i, NotAlloc, lifetime);
   itmp_status[i] = InUse;
   if (num_itmp < i + 1)
     num_itmp = i + 1;
   return i;
   }

/*
 * alc_dtmp - allocate a temporary C integer variable.
 */
int alc_dtmp(lifetime)
nodeptr lifetime;
   {
   int i, j;
   int new_size;

   i = 0;
   while (i < dstatus_sz && dtmp_status[i] == InUse)
      ++i;
   if (i >= dstatus_sz) {
      /*
       * The status array is too small, expand it.
       */
      free((char *)dtmp_status);
      new_size = dstatus_sz * 2;
      dtmp_status = (int *)alloc((unsigned int)(new_size * sizeof(int)));
      j = 0;
      while (j < dstatus_sz)
         dtmp_status[j++] = InUse;
      while (j < new_size)
         dtmp_status[j++] = NotAlloc;
      dstatus_sz = new_size;
      }
   endlife(CDblTmp, i, NotAlloc, lifetime);
   dtmp_status[i] = InUse;
   if (num_dtmp < i + 1)
     num_dtmp = i + 1;
   return i;
   }

/*
 * alc_sbufs - allocate a block of string buffers with the given lifetime.
 */
int alc_sbufs(num, lifetime)
int num;
nodeptr lifetime;
   {
   int i, j, k;
   int *new_status;
   int new_size;

   i = 0;
   for (;;) {
      if (i + num > sstatus_sz) {
         /*
          * The status array is too small, expand it.
          */
         new_size = sstatus_sz + Max(num, sstatus_sz);
         new_status = (int *)alloc((unsigned int)(new_size * sizeof(int)));
         k = 0;
         while (k < sstatus_sz) {
            new_status[k] = sbuf_status[k];
            ++k;
            }
         while (k < new_size) {
            new_status[k] = NotAlloc;
            ++k;
            }
         free((char *)sbuf_status);
         sbuf_status = new_status;
         sstatus_sz = new_size;
         }
      for (j = 0; j < num && sbuf_status[i + j] == NotAlloc; ++j)
         ;
      /*
       * Did we find a block of buffers that we can use?
       */
      if (j == num) {
         while (--j >= 0) {
            endlife(SBuf, i + j, sbuf_status[i + j], lifetime);
            sbuf_status[i + j] = InUse;
            }
         if (i + num > num_sbuf)
            num_sbuf = i + num;
         return i;
         }
      ++i;
      }
   }

/*
 * alc_cbufs - allocate a block of cset buffers with the given lifetime.
 */
int alc_cbufs(num, lifetime)
int num;
nodeptr lifetime;
   {
   int i, j, k;
   int *new_status;
   int new_size;

   i = 0;
   for (;;) {
      if (i + num > cstatus_sz) {
         /*
          * The status array is too small, expand it.
          */
         new_size = cstatus_sz + Max(num, cstatus_sz);
         new_status = (int *)alloc((unsigned int)(new_size * sizeof(int)));
         k = 0;
         while (k < cstatus_sz) {
            new_status[k] = cbuf_status[k];
            ++k;
            }
         while (k < new_size) {
            new_status[k] = NotAlloc;
            ++k;
            }
         free((char *)cbuf_status);
         cbuf_status = new_status;
         cstatus_sz = new_size;
         }
      for (j = 0; j < num && cbuf_status[i + j] == NotAlloc; ++j)
         ;
      /*
       * Did we find a block of buffers that we can use?
       */
      if (j == num) {
         while (--j >= 0) {
            endlife(CBuf, i + j, cbuf_status[i + j], lifetime);
            cbuf_status[i + j] = InUse;
            }
         if (i + num > num_cbuf)
            num_cbuf = i + num;
         return i;
         }
      ++i;
      }
   }
