/*
 * codegen.c - routines to write out C code.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "cglobals.h"
#include "csym.h"
#include "ccode.h"
#include "ctree.h"
#include "cproto.h"

#ifndef LoopThreshold
#define LoopThreshold 7
#endif					/* LoopThreshold */

/*
 * MinOne - arrays sizes must be at least 1.
 */
#define MinOne(n) ((n) > 0 ? (n) : 1)

/*
 * ChkSeqNum - make sure a label has been given a sequence number.
 */
#define ChkSeqNum(x) if ((x)->SeqNum == 0) (x)->SeqNum = ++lbl_seq_num

/*
 * ChkBound - for a given procedure, signals that transfer control to a
 *  bounding label all use the same signal number.
 */
#define ChkBound(x) (((x)->LabFlg & Bounding) ? bound_sig : (x))

/*
 * When a switch statement for signal handling is optimized, there
 *  are three possible forms of default clauses.
 */
#define DfltNone    0   /* no default clause */
#define DfltBrk     1   /* default is just a break */
#define DfltRetSig  2   /* default is to return the signal from the call */

/*
 * Prototypes for static functions.
 */
static int     arg_nms   (struct lentry *lptr, int prt);
static void bi_proc   (char *name, struct implement *ip);
static void chkforgn  (int outer);
static int     dyn_nms   (struct lentry *lptr, int prt);
static void fldnames  (struct fldname *fields);
static void fnc_blk   (struct gentry *gptr);
static void frame     (int outer);
static void good_clsg (struct code *call, int outer);
static void initpblk  (FILE *f, int c, char *prefix, char *name,
                           int nquals, int nparam, int ndynam, int nstatic,
                           int frststat);
static char  *is_builtin (struct gentry *gptr);
static void proc_blk  (struct gentry *gptr, int init_glbl);
static void prt_ary   (struct code *cd, int outer);
static void prt_cond  (struct code *cond);
static void prt_cont  (struct c_fnc *cont);
static void prt_var   (struct lentry *var, int outer);
static void prtcall   (struct code *call, int outer);
static void prtcode   (struct code *cd, int outer);
static void prtpccall (int outer);
static void rec_blk   (struct gentry *gptr, int init_glbl);
static void smpl_clsg (struct code *call, int outer);
static void stat_nms  (struct lentry *lptr, int prt);
static void val_loc   (struct val_loc *rslt, int outer);

static int n_stat = -1;		/* number of static variables */

/*
 * var_dcls - produce declarations necessary to implement variables
 *  and to initialize globals and statics: procedure blocks, procedure
 *  frames, record blocks, declarations for globals and statics, the
 *  C main program.
 */
void var_dcls()
   {
   register int i;
   register struct gentry *gptr;
   struct gentry *gbl_main;
   struct pentry *prc_main;
   int n_glob = 0;
   int flag;
   int init_glbl;
   int first;
   char *pfx;

   /*
    * Output initialized array of descriptors for globals.
    */
   fprintf(codefile, "\nstatic struct {word dword; union block *vword;}");
   fprintf(codefile, " init_globals[NGlobals] = {\n");
   prc_main = NULL;
   for (i = 0; i < GHSize; i++)
      for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink) {
         flag = gptr->flag & ~(F_Global | F_StrInv);
         if (strcmp(gptr->name, "main") == 0 && (gptr->flag & F_Proc)) {
            /*
             * Remember main procedure.
             */
            gbl_main = gptr;
            prc_main = gbl_main->val.proc;
            }
         if (flag == 0) {
            /*
             * Ordinary variable.
             */
            gptr->index = n_glob++;
            fprintf(codefile, "   {D_Null},\n");
            }
         else {
            /*
             * Procedure, function, or record constructor. If the variable
             *  has not been optimized away, initialize the it to reference
             *  the procedure block.
             */
            if (flag & F_SmplInv) {
               init_glbl = 0;
               flag &= ~(uword)F_SmplInv;
               }
            else {
               init_glbl = 1;
               gptr->index = n_glob++;
               fprintf(codefile, "   {D_Proc, ");
               }
            switch (flag) {
               case F_Proc:
                  proc_blk(gptr, init_glbl);
                  break;
               case F_Builtin:
                  if (init_glbl)
                     fnc_blk(gptr);
                  break;
               case F_Record:
                  rec_blk(gptr, init_glbl);
               }
            }
         }
   if (n_glob == 0)
      fprintf(codefile, "   {D_Null} /* place holder */\n");
   fprintf(codefile, "   };\n");

   if (prc_main == NULL) {
      nfatal(NULL, "main procedure missing", NULL);
      return;
      }

   /*
    * Output array of descriptors initialized to the names of the
    *  global variables that have not been optimized away.
    */
   if (n_glob == 0)
      fprintf(codefile, "\nstruct sdescrip init_gnames[1];\n");
   else {
      fprintf(codefile, "\nstruct sdescrip init_gnames[NGlobals] = {\n");
      for (i = 0; i < GHSize; i++)
         for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink)
            if (!(gptr->flag & F_SmplInv))
               fprintf(codefile, "   {%d, \"%s\"},\n", strlen(gptr->name),
                  gptr->name);
         fprintf(codefile, "   };\n");
      }

   /*
    * Output array of pointers to builtin functions that correspond to
    *  names of the global variables.
    */
   if (n_glob == 0)
      fprintf(codefile, "\nstruct b_proc *builtins[1];\n");
   else {
      fprintf(codefile, "\nstruct b_proc *builtins[NGlobals] = {\n");
      for (i = 0; i < GHSize; i++)
         for (gptr = ghash[i]; gptr != NULL; gptr = gptr->blink)
            if (!(gptr->flag & F_SmplInv)) {  
               /*
                * Need to output *something* to stay in step with other arrays.
                */
               if (pfx = is_builtin(gptr)) {
                  fprintf(codefile, "   (struct b_proc *)&BF%c%c_%s,\n",
                     pfx[0], pfx[1], gptr->name);
                  }
               else
                  fprintf(codefile, "   0,\n");
               }
         fprintf(codefile, "   };\n");
      }

   /*
    * Output C main function that initializes the run-time system and
    *  calls the main procedure.
    */
   fprintf(codefile, "\n");
   fprintf(codefile, "int main(argc, argv)\n");
   fprintf(codefile, "int argc;\n");
   fprintf(codefile, "char **argv;\n");
   fprintf(codefile, "   {\n");

   /*
    *  If the main procedure requires a command-line argument list, we
    *  need a place to construct the Icon argument list.
    */
   if (prc_main->nargs != 0 || !(gbl_main->flag & F_SmplInv)) {
      fprintf(codefile, "   struct {\n");
      fprintf(codefile, "      struct tend_desc *previous;\n");
      fprintf(codefile, "      int num;\n");
      fprintf(codefile, "      struct descrip arg_lst;\n");
      fprintf(codefile, "      } t;\n");
      fprintf(codefile, "\n");
      }

   /*
    * Produce code to initialize run-time system variables. Some depend
    *  on compiler options.
    */
   fprintf(codefile, "   op_tbl = (struct b_proc *)init_op_tbl;\n");
   fprintf(codefile, "   globals = (dptr)init_globals;\n");
   fprintf(codefile, "   eglobals = &globals[%d];\n", n_glob);
   fprintf(codefile, "   gnames = (dptr)init_gnames;\n");
   fprintf(codefile, "   egnames = &gnames[%d];\n", n_glob);
   fprintf(codefile, "   estatics = &statics[%d];\n", n_stat + 1);
   if (debug_info)
      fprintf(codefile, "   debug_info = 1;\n");
   else
      fprintf(codefile, "   debug_info = 0;\n");
   if (line_info) {
      fprintf(codefile, "   line_info = 1;\n");
      fprintf(codefile, "   file_name = \"\";\n");
      fprintf(codefile, "   line_num = 0;\n");
      }
   else
      fprintf(codefile, "   line_info = 0;\n");
   if (err_conv)
      fprintf(codefile, "   err_conv = 1;\n");
   else
      fprintf(codefile, "   err_conv = 0;\n");
   if (largeints)
      fprintf(codefile, "   largeints = 1;\n");
   else
      fprintf(codefile, "   largeints = 0;\n");

   /*
    * Produce code to call the routine to initialize the runtime system.
    */
   if (trace)
      fprintf(codefile, "   init(*argv, &argc, argv, -1);\n");
   else
      fprintf(codefile, "   init(*argv, &argc, argv, 0);\n");
   fprintf(codefile, "\n");

   /*
    * If the main procedure requires an argument list (perhaps because
    *  it uses standard, rather than tailored calling conventions),
    *  set up the argument list.
    */
   if (prc_main->nargs != 0 || !(gbl_main->flag & F_SmplInv)) {
      fprintf(codefile, "   t.arg_lst = nulldesc;\n");
      fprintf(codefile, "   t.num = 1;\n");
      fprintf(codefile, "   t.previous = NULL;\n");
      fprintf(codefile, "   tend = (struct tend_desc *)&t;\n");
      if (prc_main->nargs == 0)
         fprintf(codefile,
            "   /* main() takes no arguments: construct no list */\n"); 
      else
         fprintf(codefile, "   cmd_line(argc, argv, &t.arg_lst);\n");
      fprintf(codefile, "\n");
      }
   else
      fprintf(codefile, "   tend = NULL;\n");

   if (gbl_main->flag & F_SmplInv) {
      /*
       * procedure main only has a simplified implementation if it
       *  takes either 0 or 1 argument.
       */
      first = 1;
      if (prc_main->nargs == 0)
         fprintf(codefile, "   P%s_main(", prc_main->prefix);
      else {
         fprintf(codefile, "   P%s_main(&t.arg_lst", prc_main->prefix);
         first = 0;
         }
      if (prc_main->ret_flag & (DoesRet | DoesSusp)) {
         if (!first)
            fprintf(codefile, ", ");
         fprintf(codefile, "&trashcan");
         first = 0;
         }
      if (prc_main->ret_flag & DoesSusp)
         fprintf(codefile, ", (continuation)NULL");
      fprintf(codefile, ");\n");
      }
   else /* the main procedure uses standard calling conventions */
      fprintf(codefile,
         "   P%s_main(1, &t.arg_lst, &trashcan, (continuation)NULL);\n",
         prc_main->prefix);
   fprintf(codefile, "   \n");
   fprintf(codefile, "   c_exit(EXIT_SUCCESS);\n");
   fprintf(codefile, "   }\n");

   /*
    * Output to header file definitions related to global and static
    *  variables.
    */
   fprintf(inclfile, "\n");
   if (n_glob == 0) {
      fprintf(inclfile, "#define NGlobals 1\n");
      fprintf(inclfile, "int n_globals = 0;\n");
      }
   else {
      fprintf(inclfile, "#define NGlobals %d\n", n_glob);
      fprintf(inclfile, "int n_globals = NGlobals;\n");
      }
   ++n_stat;
   fprintf(inclfile, "\n");
   fprintf(inclfile, "int n_statics = %d;\n", n_stat);
   fprintf(inclfile, "struct descrip statics[%d]", MinOne(n_stat));
   if (n_stat > 0)  {
      fprintf(inclfile, " = {\n");
      for (i = 0; i < n_stat; ++i)
         fprintf(inclfile, "   {D_Null},\n");
      fprintf(inclfile, "   };\n");
      }
   else
      fprintf(inclfile, ";\n");
   }

/*
 * proc_blk - create procedure block and initialize global variable, also
 *   compute offsets for local procedure variables.
 */
static void proc_blk(gptr, init_glbl)
struct gentry *gptr;
int init_glbl;
   {
   struct pentry *p;
   register char *name;
   int nquals;

   name = gptr->name;
   p = gptr->val.proc;

   /*
    * If we don't initialize a global variable for this procedure, we
    *  need only compute offsets for variables.
    */
   if (init_glbl) {
      fprintf(codefile, "(union block *)&BP%s_%s},\n", p->prefix, name);
      nquals = 1 + Abs(p->nargs) + p->ndynam + p->nstatic;
      fprintf(inclfile, "\n");
      fprintf(inclfile, "static int P%s_%s (int r_nargs, dptr r_args,",
         p->prefix, name);
      fprintf(inclfile, "dptr r_rslt, continuation r_s_cont);\n");
      initpblk(inclfile, 'P', p->prefix, name, nquals, p->nargs, p->ndynam,
         p->nstatic, n_stat + 1);
      fprintf(inclfile, "\n   {%d, \"%s\"},\n", strlen(name), name);
      }
   arg_nms(p->args, init_glbl);
   p->tnd_loc = dyn_nms(p->dynams, init_glbl);
   stat_nms(p->statics, init_glbl);
   if (init_glbl)
      fprintf(inclfile, "   }};\n");
   }

/*
 * arg_nms - compute offsets of arguments and, if needed, output the
 *  initializer for a descriptor for the argument name.
 */
static int arg_nms(lptr, prt)
struct lentry *lptr;
int prt;
   {
   register int n;

   if (lptr == NULL)
      return 0;
   n = arg_nms(lptr->next, prt);
   lptr->val.index = n;
   if (prt)
      fprintf(inclfile, "   {%d, \"%s\"},\n", strlen(lptr->name), lptr->name);
   return n + 1;
   }

/*
 * dyn_nms - compute offsets of dynamic locals and, if needed, output the
 *  initializer for a descriptor for the variable name.
 */
static int dyn_nms(lptr, prt)
struct lentry *lptr;
int prt;
   {
   register int n;

   if (lptr == NULL)
      return 0;
   n = dyn_nms(lptr->next, prt);
   lptr->val.index = n;
   if (prt)
      fprintf(inclfile, "   {%d, \"%s\"},\n", strlen(lptr->name), lptr->name);
   return n + 1;
   }

/*
 * stat_nams - compute offsets of static locals and, if needed, output the
 *  initializer for a descriptor for the variable name.
 */
static void stat_nms(lptr, prt)
struct lentry *lptr;
int prt;
   {
   if (lptr == NULL)
      return;
   stat_nms(lptr->next, prt);
   lptr->val.index = ++n_stat;
   if (prt)
      fprintf(inclfile, "   {%d, \"%s\"},\n", strlen(lptr->name), lptr->name);
   }

/*
 * is_builtin - check if a global names or hides a builtin, returning prefix.
 *  If it hides one, we must also generate the prototype and block here.
 */
static char *is_builtin(gptr)
struct gentry *gptr;
   {
   struct implement *iptr;

   if (!(gptr->flag & F_StrInv))	/* if not eligible for string invoc */
      return 0;
   if (gptr->flag & F_Builtin)		/* if global *is* a builtin */
      return gptr->val.builtin->prefix;
   iptr = db_ilkup(gptr->name, bhash);
   if (iptr == NULL)			/* if no builtin by this name */
      return NULL;
   bi_proc(gptr->name, iptr);		/* output prototype and proc block */
   return iptr->prefix;
   }

/*
 * fnc_blk - output vword of descriptor for a built-in function and its
 *   procedure block.
 */
static void fnc_blk(gptr)
struct gentry *gptr;
   {
   struct implement *iptr;
   char *name, *pfx;
   
   name = gptr->name;
   iptr = gptr->val.builtin;
   pfx = iptr->prefix;
   /*
    * output prototype and procedure block to inclfile.
    */
   bi_proc(name, iptr);
   /*
    * vword of descriptor references the procedure block.
    */
   fprintf(codefile, "(union block *)&BF%c%c_%s}, \n", pfx[0], pfx[1], name);
   }

/*
 * bi_proc - output prototype and procedure block for builtin function.
 */
static void bi_proc(name, ip)
char *name;
   struct implement *ip;
   {
   int nargs;
   char prefix[3];

   prefix[0] = ip->prefix[0];
   prefix[1] = ip->prefix[1];
   prefix[2] = '\0';
   nargs = ip->nargs;
   if (nargs > 0 && ip->arg_flgs[nargs - 1] & VarPrm)
      nargs = -nargs;
   fprintf(inclfile, "\n");
   implproto(ip);
   initpblk(inclfile, 'F', prefix, name, 1, nargs, -1, 0, 0);
   fprintf(inclfile, "{%d, \"%s\"}}};\n", strlen(name), name);
   }

/*
 * rec_blk - if needed, output vword of descriptor for a record
 *   constructor and output its procedure block.
 */
static void rec_blk(gptr, init_glbl)
struct gentry *gptr;
int init_glbl;
   {
   struct rentry *r;
   register char *name;
   int nfields;

   name = gptr->name;
   r = gptr->val.rec;
   nfields = r->nfields;

   /*
    * If the variable is not optimized away, output vword of descriptor.
    */
   if (init_glbl)
      fprintf(codefile, "(union block *)&BR%s_%s},\n", r->prefix, name);

   fprintf(inclfile, "\n");
   /*
    * Prototype for C function implementing constructor. If no optimizations
    *   have been performed on the variable, the standard calling conventions
    *   are used and we need a continuation parameter.
    */
   fprintf(inclfile,
      "static int R%s_%s (int r_nargs, dptr r_args, dptr r_rslt",
      r->prefix, name);
   if (init_glbl)
      fprintf(inclfile, ", continuation r_s_cont");
   fprintf(inclfile, ");\n");

   /*
    * Procedure block, including record name and field names.
    */
   initpblk(inclfile, 'R', r->prefix, name, nfields + 1, nfields, -2,
      r->rec_num, 1);
   fprintf(inclfile, "\n   {%d, \"%s\"},\n", strlen(name), name);
   fldnames(r->fields);
   fprintf(inclfile, "   }};\n");
   }


/*
 * fldnames - output the initializer for a descriptor for the field name.
 */
static void fldnames(fields)
struct fldname *fields;
   {
   register char *name;

   if (fields == NULL)
      return;
   fldnames(fields->next);
   name = fields->name;
   fprintf(inclfile, "   {%d, \"%s\"},\n", strlen(name), name);
   }

/*
 * implproto - print prototype for function implementing a run-time operation.
 */
void implproto(ip)
struct implement *ip;
   {
   if (ip->iconc_flgs & ProtoPrint)
      return;    /* only print prototype once */
   fprintf(inclfile, "int %c%c%c_%s ", ip->oper_typ, ip->prefix[0],
       ip->prefix[1], ip->name);
   fprintf(inclfile, "(int r_nargs, dptr r_args, dptr r_rslt, ");
   fprintf(inclfile,"continuation r_s_cont);\n");
   ip->iconc_flgs |= ProtoPrint;
   }

/*
 * const_blks - output blocks for cset and real constants.
 */
void const_blks()
   {
   register int i;
   register struct centry *cptr;

   fprintf(inclfile, "\n");
   for (i = 0; i < CHSize; i++)
      for (cptr = chash[i]; cptr != NULL; cptr = cptr->blink) {
         switch (cptr->flag) {
            case F_CsetLit:
               nxt_pre(cptr->prefix, pre, PrfxSz);
               cptr->prefix[PrfxSz] = '\0';
               fprintf(inclfile, "struct b_cset BDC%s = ", cptr->prefix);
               cset_init(inclfile, cptr->u.cset);
               break;
            case F_RealLit:
               nxt_pre(cptr->prefix, pre, PrfxSz);
               cptr->prefix[PrfxSz] = '\0';
               fprintf(inclfile, "struct b_real BDR%s = {T_Real, %s};\n",
                   cptr->prefix, cptr->image);
               break;
            }
         }
   }

/*
 * reccnstr - output record constructors.
 */
void recconstr(r)
struct rentry *r;
   {
   register char *name;
   int optim;
   int nfields;

   if (r == NULL)
      return;
   recconstr(r->next);

   name = r->name;
   nfields = r->nfields;

   /*
    * Does this record constructor use optimized calling conventions?
    */
   optim = glookup(name)->flag & F_SmplInv;

   fprintf(codefile, "\n");
   fprintf(codefile, "static int R%s_%s(r_nargs, r_args, r_rslt", r->prefix,
      name);
   if (!optim)
      fprintf(codefile, ", r_s_cont");  /* continuation is passed */
   fprintf(codefile, ")\n");
   fprintf(codefile, "int r_nargs;\n");
   fprintf(codefile, "dptr r_args;\n");
   fprintf(codefile, "dptr r_rslt;\n");
   if (!optim)
      fprintf(codefile, "continuation r_s_cont;\n");
   fprintf(codefile, "   {\n");
   fprintf(codefile, "   register int i;\n");
   fprintf(codefile, "   register struct b_record *rp;\n");
   fprintf(codefile, "\n");
   fprintf(codefile, "   rp = alcrecd(%d, (union block *)&BR%s_%s);\n",
      nfields, r->prefix, name);
   fprintf(codefile, "   if (rp == NULL) {\n");
   fprintf(codefile, "      err_msg(307, NULL);\n");
   if (err_conv)
      fprintf(codefile, "      return A_Resume;\n");
   fprintf(codefile, "      }\n");
   fprintf(codefile, "   for (i = %d; i >= 0; i--)\n", nfields - 1);
   fprintf(codefile, "      if (i < r_nargs)\n");
   fprintf(codefile, "         deref(&r_args[i], &rp->fields[i]);\n");
   fprintf(codefile, "      else\n");
   fprintf(codefile, "         rp->fields[i] = nulldesc;\n");
   fprintf(codefile, "   r_rslt->vword.bptr = (union block *)rp;\n");
   fprintf(codefile, "   r_rslt->dword = D_Record;\n");
   fprintf(codefile, "   return A_Continue;\n");
   fprintf(codefile, "   }\n");
   }

/*
 * outerfnc - output code for the outer function implementing a procedure.
 */
void outerfnc(fnc)
struct c_fnc *fnc;
   {
   char *prefix;
   char *name;
   char *cnt_var;
   char *sep;
   int ntend;
   int first_arg;
   int nparms;
   int optim; /* optimized interface: no arg list adjustment */
   int ret_flag;
#ifdef OptimizeLoop
   int i;
#endif					/* OptimizeLoop */

   prefix = cur_proc->prefix;
   name = cur_proc->name;
   ntend = cur_proc->tnd_loc + num_tmp;
   ChkPrefix(fnc->prefix);
   optim = glookup(name)->flag & F_SmplInv;
   nparms = Abs(cur_proc->nargs);
   ret_flag = cur_proc->ret_flag;

   fprintf(codefile, "\n");
   if (optim) {
      /*
       * Arg list adjustment and dereferencing are done at call site.
       *  Use simplified interface. Output both function header and
       *  prototype.
       */
      sep = "";
      fprintf(inclfile, "static int P%s_%s (", prefix, name);
      fprintf(codefile, "static int P%s_%s(", prefix, name);
      if (nparms != 0) {
         fprintf(inclfile, "dptr r_args");
         fprintf(codefile, "r_args");
         sep = ", ";
         }
      if (ret_flag & (DoesRet | DoesSusp)) {
         fprintf(inclfile, "%sdptr r_rslt", sep);
         fprintf(codefile, "%sr_rslt", sep);
         sep = ", ";
         }
      if (ret_flag & DoesSusp) {
         fprintf(inclfile, "%scontinuation r_s_cont", sep);
         fprintf(codefile, "%sr_s_cont", sep);
         sep = ", ";
         }
      if (*sep == '\0')
         fprintf(inclfile, "void");
      fprintf(inclfile, ");\n");
      fprintf(codefile, ")\n");
      if (nparms != 0)
         fprintf(codefile, "dptr r_args;\n");
      if (ret_flag & (DoesRet | DoesSusp))
         fprintf(codefile, "dptr r_rslt;\n");
      if (ret_flag & DoesSusp)
         fprintf(codefile, "continuation r_s_cont;\n");
      }
   else {
      /*
       * General invocation interface. Output function header; prototype has
       *  already been produced.
       */
      fprintf(codefile,
         "static int P%s_%s(r_nargs, r_args, r_rslt, r_s_cont)\n", prefix,
         name);
      fprintf(codefile, "int r_nargs;\n");
      fprintf(codefile, "dptr r_args;\n");
      fprintf(codefile, "dptr r_rslt;\n");
      fprintf(codefile, "continuation r_s_cont;\n");
      }

   fprintf(codefile, "{\n");
   fprintf(codefile, "   struct PF%s_%s r_frame;\n", prefix, name);
   fprintf(codefile, "   register int r_signal;\n");
   fprintf(codefile, "   int i;\n");
   if (Type(Tree1(cur_proc->tree)) != N_Empty)
      fprintf(codefile, "   static int first_time = 1;");
   fprintf(codefile, "\n");
   fprintf(codefile, "   r_frame.old_pfp = pfp;\n");
   fprintf(codefile, "   pfp = (struct p_frame *)&r_frame;\n");
   fprintf(codefile, "   r_frame.old_argp = glbl_argp;\n");
   if (!optim || ret_flag & (DoesRet | DoesSusp))
      fprintf(codefile, "   r_frame.rslt = r_rslt;\n");
   else
      fprintf(codefile, "   r_frame.rslt = NULL;\n");
   if (!optim || ret_flag & DoesSusp)
      fprintf(codefile, "   r_frame.succ_cont = r_s_cont;\n");
   else
      fprintf(codefile, "   r_frame.succ_cont = NULL;\n");
   fprintf(codefile, "\n");
#ifdef OptimizeLoop
   if (ntend > 0)  {
      if (ntend < LoopThreshold)
         for (i=0; i < ntend ;i++)  
            fprintf(codefile, "   r_frame.tend.d[%d] = nulldesc;\n", i);
      else {
         fprintf(codefile, "   for (i = 0; i < %d; ++i)\n", ntend);
         fprintf(codefile, "      r_frame.tend.d[i] = nulldesc;\n");
      }
   }
#else					/* OptimizeLoop */
   fprintf(codefile, "   for (i = 0; i < %d; ++i)\n", ntend);
   fprintf(codefile, "      r_frame.tend.d[i] = nulldesc;\n");
#endif					/* OptimizeLoop */
   if (optim) {
      /*
       * Dereferencing and argument list adjustment is done at the call
       *  site. There is not much to do here.
       */
      if (nparms == 0)
         fprintf(codefile, "   glbl_argp = NULL;\n");
      else
         fprintf(codefile, "   glbl_argp = r_args;\n");
      }
   else {
      /*
       * Dereferencing and argument list adjustment must be done by
       *  the procedure itself.
       */
      first_arg = ntend;
      ntend += nparms;
      if (cur_proc->nargs < 0) {
         /*
          * varargs - construct a list into the last argument.
          */
         nparms -= 1;
         if (nparms == 0)
            cnt_var = "r_nargs";
         else {
            fprintf(codefile, "   i = r_nargs - %d;\n", nparms);
            cnt_var = "i";
            }
         fprintf(codefile,"   if (%s <= 0)\n", cnt_var);
         fprintf(codefile,"       varargs(NULL, 0, &r_frame.tend.d[%d]);\n",
            first_arg + nparms);
         fprintf(codefile,"   else\n");
         fprintf(codefile,
            "       varargs(&r_args[%d], %s, &r_frame.tend.d[%d]);\n", nparms,
            cnt_var, first_arg + nparms);
         }
      if (nparms > 0) {
         /*
          * Output code to dereference argument or supply default null
          *  value.
          */
#ifdef OptimizeLoop
         fprintf(codefile, "   for (i = 0; i < r_nargs ; ++i)\n");
         fprintf(codefile, "      deref(&r_args[i], &r_frame.tend.d[i + %d]);\n",  first_arg);
         fprintf(codefile, "   for(i = r_nargs; i < %d ; ++i)\n", nparms);
         fprintf(codefile, "      r_frame.tend.d[i + %d] = nulldesc;\n",
            first_arg);
#else					/* OptimizeLoop */
         fprintf(codefile, "   for (i = 0; i < %d; ++i)\n", nparms);
         fprintf(codefile, "      if (i < r_nargs)\n");
         fprintf(codefile,
            "         deref(&r_args[i], &r_frame.tend.d[i + %d]);\n",
            first_arg);
         fprintf(codefile, "      else\n");
         fprintf(codefile, "         r_frame.tend.d[i + %d] = nulldesc;\n",
            first_arg);
#endif					/* OptimizeLoop */
         }
      fprintf(codefile, "   glbl_argp = &r_frame.tend.d[%d];\n", first_arg);
      }
   fprintf(codefile, "   r_frame.tend.num = %d;\n", ntend);
   fprintf(codefile, "   r_frame.tend.previous = tend;\n");
   fprintf(codefile, "   tend = (struct tend_desc *)&r_frame.tend;\n");
   if (line_info) {
      fprintf(codefile, "   r_frame.debug.old_line = line_num;\n");
      fprintf(codefile, "   r_frame.debug.old_fname = file_name;\n");
      }
   if (debug_info) {
      fprintf(codefile, "   r_frame.debug.proc = (struct b_proc *)&BP%s_%s;\n",
         prefix, name);
      fprintf(codefile, "   if (k_trace) ctrace();\n");
      fprintf(codefile, "   ++k_level;\n\n");
      }
   fprintf(codefile, "\n");

   /*
    * Output definition for procedure frame.
    */
   prt_frame(prefix, ntend, num_itmp, num_dtmp, num_sbuf, num_cbuf);

   /*
    * Output code to implement procedure body.
    */
   prtcode(&(fnc->cd), 1);
   fprintf(codefile, "   }\n");
   }

/*
 * prt_fnc - output C function that implements a continuation.
 */
void prt_fnc(fnc)
struct c_fnc *fnc;
   {
   struct code *sig;
   char *name;
   char *prefix;

   if (fnc->flag & CF_SigOnly) {
      /*
       * This function only returns a signal. A shared function is used in
       *  its place. Make sure that function has been printed.
       */
      sig = fnc->cd.next->SigRef->sig;
      if (sig->cd_id != C_Resume) {
         sig = ChkBound(sig);
         if (!(sig->LabFlg & FncPrtd)) {
            ChkSeqNum(sig);
            fprintf(inclfile, "static int sig_%d (void);\n",
               sig->SeqNum);
   
            fprintf(codefile, "\n");
            fprintf(codefile, "static int sig_%d()\n", sig->SeqNum);
            fprintf(codefile, "   {\n");
            fprintf(codefile, "   return %d; /* %s */\n", sig->SeqNum,
               sig->Desc);
            fprintf(codefile, "   }\n");
            sig->LabFlg |= FncPrtd;
            }
         }
      }
   else {
      ChkPrefix(fnc->prefix);
      prefix = fnc->prefix;
      name = cur_proc->name;
   
      fprintf(inclfile, "static int P%s_%s (void);\n", prefix, name);
   
      fprintf(codefile, "\n");
      fprintf(codefile, "static int P%s_%s()\n", prefix, name);
      fprintf(codefile, "   {\n");
      if (fnc->flag & CF_Coexpr)
         fprintf(codefile, "#ifdef Coexpr\n");
   
      prefix = fnc->frm_prfx;
   
      fprintf(codefile, "   register int r_signal;\n");
      fprintf(codefile, "   register struct PF%s_%s *r_pfp;\n", prefix, name);
      fprintf(codefile, "\n");
      fprintf(codefile, "   r_pfp  = (struct PF%s_%s *)pfp;\n", prefix, name);
      prtcode(&(fnc->cd), 0);
      if (fnc->flag & CF_Coexpr) {
         fprintf(codefile, "#else\t\t\t\t\t/* Coexpr */\n");
         fprintf(codefile, "   fatalerr(401, NULL);\n");
         fprintf(codefile, "#endif\t\t\t\t\t/* Coexpr */\n");
         }
      fprintf(codefile, "   }\n");
      }
   }

/*
 * prt_frame - output the definition for a procedure frame.
 */
void prt_frame(prefix, ntend, n_itmp, n_dtmp, n_sbuf, n_cbuf)
char *prefix;
int ntend;
int n_itmp;
int n_dtmp;
int n_sbuf;
int n_cbuf;
   {
   int i;

   /*
    * Output standard part of procedure frame including tended
    *  descriptors.
     */
   fprintf(inclfile, "\n");
   fprintf(inclfile, "struct PF%s_%s {\n", prefix, cur_proc->name);
   fprintf(inclfile, "   struct p_frame *old_pfp;\n");
   fprintf(inclfile, "   dptr old_argp;\n");
   fprintf(inclfile, "   dptr rslt;\n");
   fprintf(inclfile, "   continuation succ_cont;\n");
   fprintf(inclfile, "   struct {\n");
   fprintf(inclfile, "      struct tend_desc *previous;\n");
   fprintf(inclfile, "      int num;\n");
   fprintf(inclfile, "      struct descrip d[%d];\n", MinOne(ntend));
   fprintf(inclfile, "      } tend;\n");

   if (line_info) {       /* must be true if debug_info is true */
      fprintf(inclfile, "   struct debug debug;\n");
      }

   /*
    * Output declarations for the integer, double, string buffer,
    *  and cset buffer work areas of the frame.
    */
   for (i = 0; i < n_itmp; ++i)
      fprintf(inclfile, "   word i%d;\n", i);
   for (i = 0; i < n_dtmp; ++i)
      fprintf(inclfile, "   double d%d;\n", i);
   if (n_sbuf > 0)
      fprintf(inclfile, "   char sbuf[%d][MaxCvtLen];", n_sbuf);
   if (n_cbuf > 0)
      fprintf(inclfile, "   struct b_cset cbuf[%d];", n_cbuf);
   fprintf(inclfile, "   };\n");
   }

/*
 * prtcode - print a list of C code.
 */
static void prtcode(cd, outer)
struct code *cd;
int outer;
   {
   struct lentry *var;
   struct centry *lit;
   struct code *sig;
   int n;

   for ( ; cd != NULL; cd = cd->next) {
      switch (cd->cd_id) {
         case C_Null:
            break;

         case C_NamedVar:
            /*
             * Construct a reference to a named variable in a result
             *  location.
             */
            var = cd->NamedVar;
            fprintf(codefile, "   ");
            val_loc(cd->Rslt, outer);
            fprintf(codefile, ".dword = D_Var;\n");
            fprintf(codefile, "   ");
            val_loc(cd->Rslt, outer);
            fprintf(codefile, ".vword.descptr = &");
            prt_var(var, outer);
            fprintf(codefile, ";\n");
            break;

         case C_CallSig:
            /*
             * Call to C function that returns a signal along with signal
             *   handling code.
             */
            if (opt_sgnl)
               good_clsg(cd, outer);
            else
               smpl_clsg(cd, outer);
            break;

         case C_RetSig:
            /*
             * Return a signal.
             */
            sig = cd->SigRef->sig;
            if (sig->cd_id == C_Resume)
               fprintf(codefile, "   return A_Resume;\n");
            else {
               sig = ChkBound(sig);
               ChkSeqNum(sig);
               fprintf(codefile, "   return %d; /* %s */\n", sig->SeqNum,
                  sig->Desc);
               }
            break;

         case C_Goto:
            /*
             * goto label.
             */
            ChkSeqNum(cd->Lbl);
            fprintf(codefile, "   goto L%d /* %s */;\n", cd->Lbl->SeqNum, 
               cd->Lbl->Desc);
            break;

         case C_Label:
            /*
             * numbered label.
             */
            if (cd->RefCnt > 0) {
               ChkSeqNum(cd);
               fprintf(codefile, "L%d: ; /* %s */\n", cd->SeqNum, cd->Desc);
               }
            break;

         case C_Lit:
            /*
             * Assign literal value to a result location.
             */
            lit = cd->Literal;
            fprintf(codefile, "   ");
            val_loc(cd->Rslt, outer);
            switch (lit->flag) {
               case F_CsetLit:
                  fprintf(codefile, ".dword = D_Cset;\n");
                  fprintf(codefile, "   ");
                  val_loc(cd->Rslt, outer);
                  fprintf(codefile, ".vword.bptr = (union block *)&BDC%s;\n",
                     lit->prefix);
                  break;
               case F_IntLit:
                  if (lit->u.intgr == -1) {
                     /*
                      * Large integer literal - output string and convert
                      *  to integer.
                      */
                     fprintf(codefile, ".vword.sptr = \"%s\";\n", lit->image);
                     fprintf(codefile, "   ");
                     val_loc(cd->Rslt, outer);
                     fprintf(codefile, ".dword = %d;\n", strlen(lit->image));
                     fprintf(codefile, "   cnv_int(&");
                     val_loc(cd->Rslt, outer);
                     fprintf(codefile, ", &");
                     val_loc(cd->Rslt, outer);
                     fprintf(codefile, ");\n");
                     }
                  else {
                     /*
                      * Ordinary integer literal.
                      */
                     fprintf(codefile, ".dword = D_Integer;\n");
                     fprintf(codefile, "   ");
                     val_loc(cd->Rslt, outer);
                     fprintf(codefile, ".vword.integr = %ld;\n", lit->u.intgr);
                     }
                  break;
               case F_RealLit:
                  fprintf(codefile, ".dword = D_Real;\n");
                  fprintf(codefile, "   ");
                  val_loc(cd->Rslt, outer);
                  fprintf(codefile, ".vword.bptr = (union block *)&BDR%s;\n",
                     lit->prefix);
                  break;
               case F_StrLit:
                  fprintf(codefile, ".vword.sptr = ");
                  if (lit->length ==  0) {
                     /*
                      * Placing an empty string at the end of the string region
                      *  allows some concatenation optimizations at run time.
                      */
                     fprintf(codefile, "strfree;\n");
                     n = 0;
                     }
                  else {
                     fprintf(codefile, "\"");
                     n = prt_i_str(codefile, lit->image, lit->length);
                     fprintf(codefile, "\";\n");
                     }
                  fprintf(codefile, "   ");
                  val_loc(cd->Rslt, outer);
                  fprintf(codefile, ".dword = %d;\n", n);
                  break;
               }
            break;

         case C_PFail:
            /*
             * Procedure failure - this code occurs once near the end of
             *  the procedure.
             */
            if (debug_info) {
               fprintf(codefile, "   --k_level;\n");
               fprintf(codefile, "   if (k_trace) failtrace();\n");
               }
            fprintf(codefile, "   tend = r_frame.tend.previous;\n");
            fprintf(codefile, "   pfp = r_frame.old_pfp;\n");
            fprintf(codefile, "   glbl_argp = r_frame.old_argp;\n");
            if (line_info) {
               fprintf(codefile, "   line_num = r_frame.debug.old_line;\n");
               fprintf(codefile, "   file_name = r_frame.debug.old_fname;\n");
               }
            fprintf(codefile, "   return A_Resume;\n");
            break;

         case C_PRet:
            /*
             * Procedure return - this code occurs once near the end of
             *  the procedure.
             */
            if (debug_info) {
               fprintf(codefile, "   --k_level;\n");
               fprintf(codefile, "   if (k_trace) rtrace();\n");
               }
            fprintf(codefile, "   tend = r_frame.tend.previous;\n");
            fprintf(codefile, "   pfp = r_frame.old_pfp;\n");
            fprintf(codefile, "   glbl_argp = r_frame.old_argp;\n");
            if (line_info) {
               fprintf(codefile, "   line_num = r_frame.debug.old_line;\n");
               fprintf(codefile, "   file_name = r_frame.debug.old_fname;\n");
               }
            fprintf(codefile, "   return A_Continue;\n");
            break;

         case C_PSusp:
            /*
             * Procedure suspend - call success continuation.
             */
            prtpccall(outer);
            break;

         case C_Break:
            fprintf(codefile, "   break;\n");
            break;

         case C_If:
            /*
             * C if statement.
             */
            fprintf(codefile, "   if (");
            prt_ary(cd->Cond, outer);
            fprintf(codefile, ")\n   ");
            prtcode(cd->ThenStmt, outer);
            break;

         case C_CdAry:
            /*
             * Array of code fragments.
             */
            fprintf(codefile, "   ");
            prt_ary(cd, outer);
            fprintf(codefile, "\n");
            break;

         case C_LBrack:
            fprintf(codefile, "   {\n");
            break;

         case C_RBrack:
            fprintf(codefile, "   }\n");
            break;

         case C_Create:
            /*
             * Code to create a co-expression and assign it to a result
             *  location.
             */
            fprintf(codefile, "   ");
            val_loc(cd->Rslt, outer);
            fprintf(codefile , ".vword.bptr = (union block *)create(");
            prt_cont(cd->Cont);
            fprintf(codefile,
               ", (struct b_proc *)&BP%s_%s, %d, sizeof(word) * %d);\n",
               cur_proc->prefix, cur_proc->name, cd->NTemps, cd->WrkSize);
            fprintf(codefile, "   ");
            val_loc(cd->Rslt, outer);
            fprintf(codefile, ".dword = D_Coexpr;\n");
            break;

         case C_SrcLoc:
            /*
             * Update file name and line number information.
             */
            if (cd->FileName != NULL) {
               fprintf(codefile, "   file_name = \"");
               prt_i_str(codefile, cd->FileName, strlen(cd->FileName));
               fprintf(codefile, "\";\n");
               }
            if (cd->LineNum != 0)
               fprintf(codefile, "   line_num = %d;\n", cd->LineNum);
            break;
         }
      }
   }

/*
 * prt_var - output C code to reference an Icon named variable.
 */
static void prt_var(var, outer)
struct lentry *var;
int outer;
   {
   switch (var->flag) {
      case F_Global:
         fprintf(codefile, "globals[%d]", var->val.global->index);
         break;
      case F_Static:
         fprintf(codefile, "statics[%d]", var->val.index);
         break;
      case F_Dynamic:
         frame(outer);
         fprintf(codefile, ".tend.d[%d]", var->val.index);
         break;
      case F_Argument:
         fprintf(codefile, "glbl_argp[%d]", var->val.index);
         }

   /*
    * Include an identifying comment.
    */
   fprintf(codefile, " /* %s */", var->name);
   }

/*
 * prt_ary - print an array of code fragments.
 */
static void prt_ary(cd, outer)
struct code *cd;
int outer;
   {
   int i;

   for (i = 0; cd->ElemTyp(i) != A_End; ++i)
      switch (cd->ElemTyp(i)) {
         case A_Str:
            /*
             * Simple C code in a string.
             */
            fprintf(codefile, "%s", cd->Str(i));
            break;
         case A_ValLoc:
            /*
             * Value location (usually variable of some sort).
             */
            val_loc(cd->ValLoc(i), outer);
            break;
         case A_Intgr:
            /*
             * Integer.
             */
            fprintf(codefile, "%d", cd->Intgr(i));
            break;
         case A_ProcCont:
            /*
             * Current procedure call's success continuation.
             */
            if (outer)
               fprintf(codefile, "r_s_cont");
            else
               fprintf(codefile, "r_pfp->succ_cont");
            break;
         case A_SBuf:
            /*
             * One of the string buffers.
             */
            frame(outer);
            fprintf(codefile, ".sbuf[%d]", cd->Intgr(i));
            break;
         case A_CBuf:
            /*
             * One of the cset buffers.
             */
            fprintf(codefile, "&(");
            frame(outer);
            fprintf(codefile, ".cbuf[%d])", cd->Intgr(i));
            break;
         case A_Ary:
            /*
             * A subarray of code fragments.
             */
            prt_ary(cd->Array(i), outer);
            break;
         }
   }

/*
 * frame - access to the procedure frame. Access directly from outer function,
 *   but access through r_pfp from a continuation.
 */
static void frame(outer)
int outer;
   {
   if (outer)
      fprintf(codefile, "r_frame");
   else
      fprintf(codefile, "(*r_pfp)");
   }

/*
 * prtpccall - print procedure continuation call.
 */
static void prtpccall(outer)
int outer;
   {
   int first_arg;
   int optim; /* optimized interface: no arg list adjustment */

   first_arg = cur_proc->tnd_loc + num_tmp;
   optim = glookup(cur_proc->name)->flag & F_SmplInv;

   /*
    * The only signal to be handled in this procedure is
    *  resumption, the rest must be passed on.
    */
   if (cur_proc->nargs != 0 && optim && !outer) {
      fprintf(codefile, "   {\n");
      fprintf(codefile, "   dptr r_argp_sav;\n");
      fprintf(codefile, "\n");
      fprintf(codefile, "   r_argp_sav = glbl_argp;\n");
      }
   if (debug_info) {
      fprintf(codefile, "   --k_level;\n");
      fprintf(codefile, "   if (k_trace) strace();\n");
      }
   fprintf(codefile, "   pfp = ");
   frame(outer);
   fprintf(codefile, ".old_pfp;\n");
   fprintf(codefile, "   glbl_argp = ");
   frame(outer);
   fprintf(codefile, ".old_argp;\n");
   if (line_info) {
      fprintf(codefile, "   line_num = ");
      frame(outer);
      fprintf(codefile, ".debug.old_line;\n");
      fprintf(codefile, "   file_name = ");
      frame(outer);
      fprintf(codefile , ".debug.old_fname;\n");
      }
   fprintf(codefile, "   r_signal = (*");
   if (outer)
      fprintf(codefile, "r_s_cont)();\n");
   else
      fprintf(codefile, "r_pfp->succ_cont)();\n");
   fprintf(codefile, "   if (r_signal != A_Resume) {\n");
   if (outer)
      fprintf(codefile, "      tend = r_frame.tend.previous;\n");
   fprintf(codefile, "      return r_signal;\n");
   fprintf(codefile, "      }\n");
   fprintf(codefile, "   pfp = (struct p_frame *)&");
   frame(outer);
   fprintf(codefile, ";\n");
   if (cur_proc->nargs == 0)
      fprintf(codefile, "   glbl_argp = NULL;\n");
   else {
      if (optim) {
         if (outer)
            fprintf(codefile, "   glbl_argp = r_args;\n");
         else
            fprintf(codefile, "   glbl_argp = r_argp_sav;\n");
         }
      else {
         fprintf(codefile, "   glbl_argp = &");
         if (outer)
            fprintf(codefile, "r_frame.");
         else
            fprintf(codefile, "r_pfp->");
         fprintf(codefile, "tend.d[%d];\n", first_arg);
         }
      }
   if (debug_info) {
      fprintf(codefile, "   if (k_trace) atrace();\n");
      fprintf(codefile, "   ++k_level;\n");
      }
   if (cur_proc->nargs != 0 && optim && !outer)
      fprintf(codefile, "   }\n");
   }

/*
 * smpl_clsg - print call and signal handling code, but nothing fancy.
 */
static void smpl_clsg(call, outer)
struct code *call;
int outer;
   {
   struct sig_act *sa;

   fprintf(codefile, "   r_signal = ");
   prtcall(call, outer);
   fprintf(codefile, ";\n");
   if (call->Flags & ForeignSig)
       chkforgn(outer);
   fprintf(codefile, "   switch (r_signal) {\n");
   for (sa = call->SigActs; sa != NULL; sa = sa->next) {
      fprintf(codefile, "      case ");
      prt_cond(sa->sig);
      fprintf(codefile, ":\n      ");
      prtcode(sa->cd, outer);
      }
   fprintf(codefile, "      }\n");
   }

/*
 * chkforgn - produce code to see if the current signal belongs to a
 *   procedure higher up the call chain and pass it along if it does.
 */
static void chkforgn(outer)
int outer;
   {
   fprintf(codefile, "   if (pfp != (struct p_frame *)");
   if (outer) {
      fprintf(codefile, "&r_frame) {\n");
      fprintf(codefile, "      tend = r_frame.tend.previous;\n");
      }
   else
      fprintf(codefile, "r_pfp) {\n");
   fprintf(codefile, "      return r_signal;\n");
   fprintf(codefile, "      }\n");
   }

/*
 * good_clsg - print call and signal handling code and do a good job.
 */
static void good_clsg(call, outer)
struct code *call;
int outer;
   {
   struct sig_act *sa, *sa1, *nxt_sa;
   int ncases;   /* the number of cases - each may have multiple case labels */
   int ncaselbl; /* the number of case labels */
   int nbreak;   /* the number of cases that just break out of the switch */
   int nretsig;  /* the number of cases that just pass along signal */
   int sig_var;
   int dflt;
   struct code *cond;
   struct code *then_cd;
   
   /*
    * Decide whether to use "break;", "return r_signal;", or nothing as
    *  the default case.
    */
   nretsig = 0;
   nbreak = 0;
   for (sa = call->SigActs; sa != NULL; sa = sa->next) {
      if (sa->cd->cd_id == C_RetSig && sa->sig == sa->cd->SigRef->sig) {
         /*
          * The action returns the same signal detected by this case.
          */
         ++nretsig;
         }
      else if (sa->cd->cd_id == C_Break) {
         cond = sa->sig;   /* if there is only one break, we may want this */
         ++nbreak;
         }
      }
   dflt = DfltNone;
   ncases = 0;
   if (nbreak > 0 && nbreak >= nretsig)  {
      /*
       * There are at least as many "break;"s as "return r_signal;"s, so
       *  use "break;" for default clause.
       */
      dflt = DfltBrk;
      ncases = 1;
      }
   else if (nretsig > 1) {
      /*
       * There is more than one case that returns the same signal it
       *  detects and there are more of them than "break;"s, to make
       *  "return r_signal;" the default clause.
       */
      dflt = DfltRetSig;
      ncases = 1;
      }

   /*
    * Gather case labels together for each case, ignoring cases that
    *  fall under the default. This involves constructing a new
    *  improved call->SigActs list.
    */
   ncaselbl = ncases;
   sa = call->SigActs;
   call->SigActs = NULL;
   for ( ; sa != NULL; sa = nxt_sa) {
      nxt_sa = sa->next;
      /*
       * See if we have already found a case with the same action.
       */
      sa1 = call->SigActs;
      switch (sa->cd->cd_id) {
         case C_Break:
            if (dflt == DfltBrk)
               continue;
            while (sa1 != NULL && sa1->cd->cd_id != C_Break)
               sa1 = sa1->next;
            break;
         case C_RetSig:
            if (dflt == DfltRetSig && sa->cd->SigRef->sig == sa->sig)
               continue;
            while (sa1 != NULL && (sa1->cd->cd_id != C_RetSig ||
                sa1->cd->SigRef->sig != sa->cd->SigRef->sig))
               sa1 = sa1->next;
            break;
         default: /* C_Goto */
            while (sa1 != NULL && (sa1->cd->cd_id != C_Goto ||
                sa1->cd->Lbl != sa->cd->Lbl))
               sa1 = sa1->next;
            break;
            }
      ++ncaselbl;
      if (sa1 == NULL) {
         /*
          * First time we have seen this action, create a new case.
          */
         ++ncases;
         sa->next = call->SigActs;
         call->SigActs = sa;
         }
      else {
         /*
          * We can share the action of another case label.
          */
         sa->shar_act = sa1->shar_act;
         sa1->shar_act = sa;
         }
      }

   /*
    * If we might receive a "foreign" signal that belongs to a procedure
    *  further down the call chain, put the signal in "r_signal" then
    *  check for this condition.
    */
   sig_var = 0;
   if (call->Flags & ForeignSig) {
      fprintf(codefile, "   r_signal = ");
      prtcall(call, outer);
      fprintf(codefile, ";\n");
      chkforgn(outer);
      sig_var = 1;
      }

   /*
    * Determine the best way to handle the signal returned from the call.
    */
   if (ncases == 0) {
      /*
       * Any further signal checking has been optimized away. Execution
       *  just falls through to subsequent code. If the call has not
       *  been done, do it.
       */
      if (!sig_var) {
         fprintf(codefile, "   ");
         prtcall(call, outer);
         fprintf(codefile, ";\n");
         }
      }
   else if (ncases == 1) {
      if (dflt == DfltRetSig || ncaselbl == nretsig) {
         /*
          * All this call does is pass the signal on. See if we have
          *  done the call yet.
          */
         if (sig_var)
            fprintf(codefile, "   return r_signal;");
         else {
            fprintf(codefile, "   return ");
            prtcall(call, outer);
            fprintf(codefile, ";\n");
            }
         }
      else {
         /*
          * We know what to do without looking at the signal. Make sure
          *  we have done the call. If the action is not simply  "break"
          *  out signal checking, execute it. 
          */
         if (!sig_var) {
            fprintf(codefile, "   ");
            prtcall(call, outer);
            fprintf(codefile, ";\n");
            }
         if (dflt != DfltBrk)
            prtcode(call->SigActs->cd, outer);
         }
      }
   else {
      /*
       * We have at least two cases. If we have a default action of returning
       *  the signal without looking at it, make sure it is in "r_signal".
       */
      if (!sig_var && dflt == DfltRetSig) {
         fprintf(codefile, "   r_signal = ");
         prtcall(call, outer);
         fprintf(codefile, ";\n");
         sig_var = 1;
         }
      
      if (ncaselbl == 2) {
         /*
          * We can use an if statement. If we need the signal in "r_signal",
          *  it is already there.
          */
         fprintf(codefile, "   if (");
         if (sig_var) 
            fprintf(codefile, "r_signal");
         else
            prtcall(call, outer);
   
         cond = call->SigActs->sig;
         then_cd = call->SigActs->cd;
            
         /*
          * If the "then" clause is a no-op ("break;" from a switch),
          *  prepare to eliminate it by reversing the test in the
          *  condition.
          */
         if (then_cd->cd_id == C_Break)
            fprintf(codefile, " != ");
         else
            fprintf(codefile, " == ");
   
         prt_cond(cond);
         fprintf(codefile, ")\n   ");
   
         if (then_cd->cd_id == C_Break) {
            /*
             * We have reversed the test, so we need to use the default
             *  code. However, because a "break;" exists and it is not
             *  default, "return r_signal;" must be the default.
             */
            fprintf(codefile, "   return r_signal;\n");
            }
         else {
            /*
             * Print the "then" clause and determine what the "else" clause
             *  is.
             */
            prtcode(then_cd, outer);
            if (call->SigActs->next != NULL) {
               fprintf(codefile, "   else\n   ");
               prtcode(call->SigActs->next->cd, outer);
               }
            else if (dflt == DfltRetSig) {
               fprintf(codefile, "   else\n");
               fprintf(codefile, "       return r_signal;\n");
               }
            }
         }
      else if (ncases == 2 && nbreak == 1) {
         /*
          * We can use an if-then statement with a negated test. Note,
          *  the non-break case is not "return r_signal" or we would have
          *  ncaselbl = 2, making the last test true. This also means that
          *  break is the default (the break condition was saved).
          */
         fprintf(codefile, "   if (");
         if (sig_var) 
            fprintf(codefile, "r_signal");
         else
            prtcall(call, outer);
         fprintf(codefile, " != ");
         prt_cond(cond);
         fprintf(codefile, ") {\n   ");
         prtcode(call->SigActs->cd, outer);
         fprintf(codefile, "      }\n");
         }
      else {
         /*
          * We must use a full case statement. If we need the signal in
          *  "r_signal", it is already there.
          */
         fprintf(codefile, "   switch (");
         if (sig_var) 
            fprintf(codefile, "r_signal");
         else
            prtcall(call, outer);
         fprintf(codefile, ") {\n");
   
         /*
          * Print the cases
          */
         for (sa = call->SigActs; sa != NULL; sa = sa->next) {
            for (sa1 = sa; sa1 != NULL; sa1 = sa1->shar_act) {
               fprintf(codefile, "      case ");
               prt_cond(sa1->sig);
               fprintf(codefile, ":\n");
               }
            fprintf(codefile, "      ");
            prtcode(sa->cd, outer);
            }
      
         /*
          * If we have a default action and it is not break, print it.
          */
         if (dflt == DfltRetSig) {
            fprintf(codefile, "      default:\n");
            fprintf(codefile, "         return r_signal;\n");
            }
      
         fprintf(codefile, "      }\n");
         }
      }
   }

/*
 * prtcall - print call.
 */
static void prtcall(call, outer)
struct code *call;
int outer;
   {
   /*
    * Either the operation or the continuation may be missing, but not
    *  both.
    */
   if (call->OperName == NULL) {
      prt_cont(call->Cont);
      fprintf(codefile, "()");
      }
   else {
      fprintf(codefile, "%s(", call->OperName);
      if (call->ArgLst != NULL)
         prt_ary(call->ArgLst, outer);
      if (call->Cont == NULL) {
         if (call->Flags & NeedCont) {
            /*
             * The operation requires a continuation argument even though
             *  this call does not include one, pass the NULL pointer.
             */
            if (call->ArgLst != NULL)
               fprintf(codefile, ", ");
            fprintf(codefile, "(continuation)NULL");
            }
         }
      else {
         /*
          * Pass the success continuation.
          */
         if (call->ArgLst != NULL)
            fprintf(codefile, ", ");
         prt_cont(call->Cont);
         }
      fprintf(codefile, ")");
      }
   }

/*
 * prt_cont - print the name of a continuation.
 */
static void prt_cont(cont)
struct c_fnc *cont;
   {
   struct code *sig;

   if (cont->flag & CF_SigOnly) {
      /*
       * This continuation only returns a signal. All continuations
       *  returning the same signal are implemented by the same C function.
       */
      sig = cont->cd.next->SigRef->sig;
      if (sig->cd_id == C_Resume)
         fprintf(codefile, "sig_rsm");
      else {
         sig = ChkBound(sig);
         ChkSeqNum(sig);
         fprintf(codefile, "sig_%d", sig->SeqNum);
         }
      }
   else {
      /*
       * Regular continuation.
       */
      ChkPrefix(cont->prefix);
      fprintf(codefile, "P%s_%s", cont->prefix, cur_proc->name);
      }
   }

/*
 * val_loc - output code referencing a value location (usually variable of
 *  some sort).
 */
static void val_loc(loc, outer)
struct val_loc *loc;
int outer;
   {
   /*
    * See if we need to cast a block pointer to a specific block type
    *  or if we need to take the address of a location.
    */
   if (loc->mod_access == M_BlkPtr && loc->blk_name != NULL)
      fprintf(codefile, "(*(struct %s **)&", loc->blk_name);
   if (loc->mod_access == M_Addr)
      fprintf(codefile, "(&");

   switch (loc->loc_type) {
      case V_Ignore:
         fprintf(codefile, "trashcan");
         break;
      case V_Temp:
         /*
          * Temporary descriptor variable.
          */
         frame(outer);
         fprintf(codefile, ".tend.d[%d]", cur_proc->tnd_loc + loc->u.tmp);
         break;
      case V_ITemp:
         /*
          * Temporary C integer variable.
          */
         frame(outer);
         fprintf(codefile, ".i%d", loc->u.tmp);
         break;
      case V_DTemp:
         /*
          * Temporary C double variable.
          */
         frame(outer);
         fprintf(codefile, ".d%d", loc->u.tmp);
         break;
      case V_Const:
         /*
          * Integer constant (used for size of variable part of arg list).
          */
         fprintf(codefile, "%d", loc->u.int_const);
         break;
      case V_NamedVar:
         /*
          * Icon named variable.
          */
         prt_var(loc->u.nvar, outer);
         break;
      case V_CVar:
         /*
          * C variable from in-line code.
          */
         fprintf(codefile, "%s", loc->u.name);
         break;
      case V_PRslt:
         /*
          * Procedure result location.
          */
         if (!outer)
            fprintf(codefile, "(*r_pfp->rslt)");
         else
            fprintf(codefile, "(*r_rslt)");
         break;
       }

   /*
    * See if we are accessing the vword of a descriptor.
    */
   switch (loc->mod_access) {
      case M_CharPtr:
         fprintf(codefile, ".vword.sptr");
         break;
      case M_BlkPtr:
         fprintf(codefile, ".vword.bptr");
         if (loc->blk_name != NULL)
            fprintf(codefile, ")");
         break;
      case M_CInt:
         fprintf(codefile, ".vword.integr");
         break;
      case M_Addr:
         fprintf(codefile, ")");
         break;
      }
   }

/*
 * prt_cond - print a condition (signal number).
 */
static void prt_cond(cond)
struct code *cond;
   {
   if (cond == &resume)
      fprintf(codefile, "A_Resume");
   else if (cond == &contin)
      fprintf(codefile, "A_Continue");
   else if (cond == &fallthru)
      fprintf(codefile, "A_FallThru");
   else {
      cond = ChkBound(cond);
      ChkSeqNum(cond);
      fprintf(codefile, "%d /* %s */", cond->SeqNum, cond->Desc);
      }
   }

/*
 * initpblk - write a procedure block along with initialization up to the
 *   the array of qualifiers.
 */
static void initpblk(f, c, prefix, name, nquals, nparam, ndynam, nstatic,
   frststat)
FILE *f;      /* output file */
int c;        /* distinguishes procedures, functions, record constructors */
char* prefix; /* prefix for name */
char *name;   /* name of routine */
int nquals;   /* number of qualifiers at end of block */
int nparam;   /* number of parameters */
int ndynam;   /* number of dynamic locals or function/record indicator */
int nstatic;  /* number of static locals or record number */
int frststat; /* index into static array of first static local */
   {
   fprintf(f, "B_IProc(%d) B%c%s_%s = ", nquals, c, prefix, name);
   fprintf(f, "{T_Proc, %d, %c%s_%s, %d, %d, %d, %d, {", 9 + 2 * nquals, c,
      prefix, name, nparam, ndynam, nstatic, frststat);
   }

