/*
 * chkinv.c - routines to determine which global names are only
 *   used as immediate operand to invocation and to directly invoke
 *   the corresponding operations. In addition, simple assignments to
 *   names variables are recognized and it is determined whether
 *   procedures return, suspend, or fail.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ctoken.h"
#include "cglobals.h"
#include "ccode.h"
#include "cproto.h"

/*
 * prototypes for static functions.
 */
static int     chg_ret  (int flag);
static void chksmpl  (struct node *n, int smpl_invk);
static int     seq_exec (int exec_flg1, int exec_flg2);
static int     spcl_inv (struct node *n, struct node *asgn);

static ret_flag;

/*
 * chkinv - check for invocation and assignment optimizations.
 */
void chkinv()
   {
   struct gentry *gp;
   struct pentry *proc;
   int exec_flg;
   int i;

   if (debug_info)
       return;  /* The following analysis is not valid */

   /*
    * start off assuming that global variables for procedure, etc. are
    *  only used as immediate operands to invocations then mark any 
    *  which are not. Any variables retaining the property are never
    *  changed. Go through the code and change invocations to such
    *  variables to invocations directly to the operation.
    */
   for (i = 0; i < GHSize; i++)
      for (gp = ghash[i]; gp != NULL; gp = gp->blink) {
         if (gp->flag & (F_Proc | F_Builtin | F_Record) &&
            !(gp->flag & F_StrInv))
               gp->flag |= F_SmplInv;
         /*
          * However, only optimize normal cases for main.
          */
         if (strcmp(gp->name, "main") == 0 && (gp->flag & F_Proc) &&
            (gp->val.proc->nargs < 0 || gp->val.proc->nargs > 1))
               gp->flag &= ~(uword)F_SmplInv;
         /*
          * Work-around to problem that a co-expression block needs
          *  block for enclosing procedure: just keep procedure in
          *  a variable to force outputting the block. Note, this
          *  inhibits tailored calling conventions for the procedure.
          */
         if ((gp->flag & F_Proc) && gp->val.proc->has_coexpr)
            gp->flag &= ~(uword)F_SmplInv;
         }

   /*
    * Analyze code in each procedure.
    */
   for (proc = proc_lst; proc != NULL; proc = proc->next) {
      chksmpl(Tree1(proc->tree), 0);  /* initial expression */
      chksmpl(Tree2(proc->tree), 0);  /* procedure body */
      }

   /*
    * Go through each procedure performing "naive" optimizations on
    *  invocations and assignments. Also determine whether the procedure
    *  returns, suspends, or fails (possibly by falling through to
    *  the end).
    */
   for (proc = proc_lst; proc != NULL; proc = proc->next) {
      ret_flag = 0;
      spcl_inv(Tree1(proc->tree), NULL);
      exec_flg = spcl_inv(Tree2(proc->tree), NULL);
      if (exec_flg & DoesFThru)
        ret_flag |= DoesFail;
      proc->ret_flag = ret_flag;
      }
   }

/*
 * smpl_invk - find any global variable uses that are not a simple
 *  invocation and mark the variables.
 */
static void chksmpl(n, smpl_invk)
struct node *n;
int smpl_invk;
   {
   struct node *cases;
   struct node *clause;
   struct lentry *var;
   int i;
   int lst_arg;

   switch (n->n_type) {
      case N_Alt:
      case N_Apply:
      case N_Limit:
      case N_Slist:
         chksmpl(Tree0(n), 0);
         chksmpl(Tree1(n), 0);
         break;

      case N_Activat:
         chksmpl(Tree1(n), 0);
         chksmpl(Tree2(n), 0);
         break;

      case N_Augop:
         chksmpl(Tree2(n), 0);
         chksmpl(Tree3(n), 0);
         break;

      case N_Bar:
      case N_Break:
      case N_Create:
      case N_Field:
      case N_Not:
         chksmpl(Tree0(n), 0);
         break;

      case N_Case:
         chksmpl(Tree0(n), 0);  /* control clause */
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
      
            chksmpl(Tree0(clause), 0);   /* value of clause */
            chksmpl(Tree1(clause), 0);   /* body of clause */
            }
         if (Tree2(n) != NULL)
            chksmpl(Tree2(n), 0);  /* default */
         break;

      case N_Cset:
      case N_Int:
      case N_Real:
      case N_Str:
      case N_Empty:
      case N_Next:
         break;

      case N_Id:
         if (!smpl_invk) {
            /*
             * The variable is being used somewhere other than in a simple
             *  invocation.
             */
            var = LSym0(n);
            if (var->flag & F_Global)
               var->val.global->flag &= ~F_SmplInv;
            }
         break;

      case N_If:
         chksmpl(Tree0(n), 0);
         chksmpl(Tree1(n), 0);
         chksmpl(Tree2(n), 0);
         break;

      case N_Invok:
         lst_arg =  1 + Val0(n);
         /*
          * Check the thing being invoked, noting that it is in fact being
          *  invoked.
          */
         chksmpl(Tree1(n), 1);
         for (i = 2; i <= lst_arg; ++i)
            chksmpl(n->n_field[i].n_ptr, 0);  /* arg i - 1 */
         break;

      case N_InvOp:
         lst_arg = 1 + Val0(n);
         for (i = 2; i <= lst_arg; ++i)
            chksmpl(n->n_field[i].n_ptr, 0);       /* arg i */
         break;

      case N_Loop: {
         switch ((int)Val0(Tree0(n))) {
            case EVERY:
            case SUSPEND:
            case WHILE:
            case UNTIL:
               chksmpl(Tree1(n), 0);   /* control clause */
               chksmpl(Tree2(n), 0);   /* do clause */
               break;

            case REPEAT:
               chksmpl(Tree1(n), 0);   /* clause */
               break;
            }
         }

      case N_Ret:
         if (Val0(Tree0(n)) == RETURN)
            chksmpl(Tree1(n), 0);
         break;

      case N_Scan:
         chksmpl(Tree1(n), 0);
         chksmpl(Tree2(n), 0);
         break;

      case N_Sect:
         chksmpl(Tree2(n), 0);
         chksmpl(Tree3(n), 0);
         chksmpl(Tree4(n), 0);
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }
   }

/*
 * spcl_inv - look for general invocations that can be replaced by
 *   special invocations. Simple assignment to a named variable is
 *   is a particularly special case. Also, determine whether execution
 *   might "fall through" this code and whether the code might fail.
 */
static int spcl_inv(n, asgn)
struct node *n;
struct node *asgn;  /* the result goes into this special-cased assignment */
   {
   struct node *cases;
   struct node *clause;
   struct node *invokee;
   struct gentry *gvar;
   struct loop {
      int exec_flg;
      struct node *asgn;
      struct loop *prev;
      } loop_info;
   struct loop *loop_sav;
   int exec_flg;
   int i;
   int lst_arg;
   static struct loop *cur_loop = NULL;

   switch (n->n_type) {
      case N_Activat:
         if (asgn != NULL)
            Val0(asgn) = AsgnDeref;  /* assume worst case */
         return seq_exec(spcl_inv(Tree1(n), NULL), spcl_inv(Tree2(n), NULL));

      case N_Alt:
         exec_flg = spcl_inv(Tree0(n), asgn) & DoesFThru;
         return exec_flg | spcl_inv(Tree1(n), asgn);

      case N_Apply:
         if (asgn != NULL)
            Val0(asgn) = AsgnCopy; /* assume worst case */
         return seq_exec(spcl_inv(Tree0(n), NULL), spcl_inv(Tree1(n), NULL));

      case N_Augop:
         exec_flg = chg_ret(Impl1(n)->ret_flag);
         if (Tree2(n)->n_type == N_Id) {
            /*
             * This is an augmented assignment to a named variable.
             *  An optimized version of assignment can be used.
             */
            n->n_type = N_SmplAug;
            if (Impl1(n)->use_rslt)
               Val0(n) = AsgnCopy;
            else
               Val0(n) = AsgnDirect;
            }
         else {
            if (asgn != NULL)
               Val0(asgn) = AsgnDeref; /* this operation produces a variable */
            exec_flg = seq_exec(exec_flg, spcl_inv(Tree2(n), NULL));
            exec_flg = seq_exec(exec_flg, chg_ret(Impl0(n)->ret_flag));
            }
         return seq_exec(exec_flg, spcl_inv(Tree3(n), NULL));

      case N_Bar:
         return spcl_inv(Tree0(n), asgn);

      case N_Break:
         if (cur_loop == NULL) {
	    nfatal(n, "invalid context for break", NULL);
            return 0;
            }
         loop_sav = cur_loop;
         cur_loop = cur_loop->prev;
         loop_sav->exec_flg |= spcl_inv(Tree0(n), loop_sav->asgn);
         cur_loop = loop_sav;
         return 0;

      case N_Create:
         spcl_inv(Tree0(n), NULL);
         return DoesFThru;

      case N_Case:
         exec_flg = spcl_inv(Tree0(n), NULL) & DoesFail; /* control clause */
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
      
            spcl_inv(Tree0(clause), NULL);
            exec_flg |= spcl_inv(Tree1(clause), asgn);
            }
         if (Tree2(n) != NULL)
            exec_flg |= spcl_inv(Tree2(n), asgn);  /* default */
         else
            exec_flg |= DoesFail;
         return exec_flg;

      case N_Cset:
      case N_Int:
      case N_Real:
      case N_Str:
      case N_Empty:
         return DoesFThru;

      case N_Field:
         if (asgn != NULL)
            Val0(asgn) = AsgnDeref;  /* operation produces variable */
         return spcl_inv(Tree0(n), NULL);

      case N_Id:
         if (asgn != NULL)
            Val0(asgn) = AsgnDeref;  /* variable */
         return DoesFThru;

      case N_If:
         spcl_inv(Tree0(n), NULL);
         exec_flg = spcl_inv(Tree1(n), asgn);
         if (Tree2(n)->n_type == N_Empty)
            exec_flg |= DoesFail;
         else
            exec_flg |= spcl_inv(Tree2(n), asgn);
         return exec_flg;

      case N_Invok:
         lst_arg = 1 + Val0(n);
         invokee = Tree1(n);
         exec_flg = DoesFThru;
         for (i = 2; i <= lst_arg; ++i)
            exec_flg = seq_exec(exec_flg, spcl_inv(n->n_field[i].n_ptr, NULL));
         if (invokee->n_type == N_Id && LSym0(invokee)->flag & F_Global) {
            /*
             * This is an invocation of a global variable. If we can
             *  convert this to a direct invocation, determine whether
             *  it is an invocation of a procedure, built-in function,
             *  or record constructor; each has a difference kind of
             *  direct invocation node.
             */
            gvar = LSym0(invokee)->val.global;
            if (gvar->flag & F_SmplInv) {
               switch (gvar->flag & (F_Proc | F_Builtin | F_Record)) {
                  case F_Proc:
                     n->n_type = N_InvProc;
                     Proc1(n) = gvar->val.proc;
                     return DoesFThru | DoesFail; /* assume worst case */
                  case F_Builtin:
                     n->n_type = N_InvOp;
                     Impl1(n) = gvar->val.builtin;
                     if (asgn != NULL && Impl1(n)->use_rslt)
                        Val0(asgn) = AsgnCopy;
                     return seq_exec(exec_flg, chg_ret(
                        gvar->val.builtin->ret_flag));
                  case F_Record:
                     n->n_type = N_InvRec;
                     Rec1(n) = gvar->val.rec;
                     return seq_exec(exec_flg, DoesFThru |
                        (err_conv ? DoesFail : 0));
                  }
               }
            }
         if (asgn != NULL)
            Val0(asgn) = AsgnCopy; /* assume worst case */
         spcl_inv(invokee, NULL);
         return DoesFThru | DoesFail; /* assume worst case */

      case N_InvOp:
         if (Impl1(n)->op != NULL && strcmp(Impl1(n)->op, ":=") == 0 &&
            Tree2(n)->n_type == N_Id) {
            /*
             * This is a simple assignment to a named variable.
             *  An optimized version of assignment can be used.
             */
            n->n_type = N_SmplAsgn;

            /* 
             * For now, assume rhs of := can compute directly into a 
             *  variable. This may be changed when the rhs is examined
             *  in the recursive call to spcl_inv().
             */
            Val0(n) = AsgnDirect;
            return spcl_inv(Tree3(n), n);
            }
         else {
            /*
             * No special cases.
             */
            lst_arg = 1 + Val0(n);
            exec_flg = chg_ret(Impl1(n)->ret_flag);
            for (i = 2; i <= lst_arg; ++i)
               exec_flg = seq_exec(exec_flg, spcl_inv(n->n_field[i].n_ptr, 
                  NULL)); /* arg i */
            if (asgn != NULL && Impl1(n)->use_rslt)
               Val0(asgn) = AsgnCopy;
            return exec_flg;
            }

      case N_Limit:
         return seq_exec(spcl_inv(Tree0(n), asgn),
            spcl_inv(Tree1(n), NULL)) | DoesFail;

      case N_Loop: {
         loop_info.prev = cur_loop;
         loop_info.exec_flg = 0;
         loop_info.asgn = asgn;
         cur_loop = &loop_info;
         switch ((int)Val0(Tree0(n))) {
            case EVERY:
            case WHILE:
            case UNTIL:
               spcl_inv(Tree1(n), NULL);   /* control clause */
               spcl_inv(Tree2(n), NULL);   /* do clause */
               exec_flg = DoesFail;
               break;

            case SUSPEND:
               spcl_inv(Tree1(n), NULL);   /* control clause */
               spcl_inv(Tree2(n), NULL);   /* do clause */
               ret_flag |= DoesSusp;
               exec_flg = DoesFail;
               break;

            case REPEAT:
               spcl_inv(Tree1(n), NULL);   /* clause */
               exec_flg = 0;
               break;
            }
         exec_flg |= cur_loop->exec_flg;
         cur_loop = cur_loop->prev;
         return exec_flg;
         }

      case N_Next:
         return 0;

      case N_Not:
         exec_flg = spcl_inv(Tree0(n), NULL);
         return ((exec_flg & DoesFail) ? DoesFThru : 0) |
            ((exec_flg & DoesFThru) ? DoesFail: 0);

      case N_Ret:
         if (Val0(Tree0(n)) == RETURN) {
            exec_flg = spcl_inv(Tree1(n), NULL);
            ret_flag |= DoesRet;
            if (exec_flg & DoesFail)
               ret_flag |= DoesFail;
            }
         else
            ret_flag |= DoesFail;
         return 0;

      case N_Scan:
         if (asgn != NULL)
            Val0(asgn) = AsgnCopy; /* assume worst case */
         return seq_exec(spcl_inv(Tree1(n), NULL),
            spcl_inv(Tree2(n), NULL));

      case N_Sect:
         if (asgn != NULL && Impl0(n)->use_rslt)
            Val0(asgn) = AsgnCopy;
         exec_flg = spcl_inv(Tree2(n), NULL);
         exec_flg = seq_exec(exec_flg, spcl_inv(Tree3(n), NULL));
         exec_flg = seq_exec(exec_flg, spcl_inv(Tree4(n), NULL));
         return seq_exec(exec_flg, chg_ret(Impl0(n)->ret_flag));

      case N_Slist:
         exec_flg = spcl_inv(Tree0(n), NULL);
         if (exec_flg & (DoesFThru | DoesFail))
            exec_flg = DoesFThru;
         return seq_exec(exec_flg, spcl_inv(Tree1(n), asgn));

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
         /* NOTREACHED */
      }
   }

/*
 * seq_exec - take the execution flags for sequential pieces of code
 *  and compute the flags for the combined code.
 */
static int seq_exec(exec_flg1, exec_flg2)
int exec_flg1;
int exec_flg2;
   {
   return (exec_flg1 & exec_flg2 & DoesFThru) |
      ((exec_flg1 | exec_flg2) & DoesFail);
   }

/*
 * chg_ret - take a return flag and change suspend and return to
 *  "fall through". If error conversion is supported, change error
 *  failure to failure.
 *  
 */
static int chg_ret(flag)
int flag;
   {
   int flg1;

   flg1 = flag & DoesFail;
   if (flag & (DoesRet | DoesSusp))
      flg1 |= DoesFThru;
   if (err_conv && (flag & DoesEFail))
      flg1 |= DoesFail;
   return flg1;
   }


