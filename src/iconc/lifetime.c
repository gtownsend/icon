/*
 * lifetime.c - perform liveness analysis to determine lifetime of intermediate
 *    results.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "cglobals.h"
#include "ctree.h"
#include "ctoken.h"
#include "csym.h"
#include "ccode.h"
#include "cproto.h"

/*
 * Prototypes for static functions.
 */
static void arg_life (nodeptr n, long min_result, long max_result,
                          int resume, int frst_arg, int nargs, nodeptr resumer,
                          nodeptr *failer, int *gen);

static int postn = -1; /* relative position in execution order (all neg) */

/*
 * liveness - compute lifetimes of intermediate results.
 */
void liveness(n, resumer, failer, gen)
nodeptr n;
nodeptr resumer;
nodeptr *failer;
int *gen;
   {
   struct loop {
      nodeptr resumer;
      int gen;
      nodeptr lifetime;
      int every_cntrl;
      struct loop *prev;
      } loop_info;
   struct loop *loop_sav;
   static struct loop *cur_loop = NULL;
   nodeptr failer1;
   nodeptr failer2;
   int gen1 = 0;
   int gen2 = 0;
   struct node *cases;
   struct node *clause;
   long min_result;  /* minimum result sequence length */
   long max_result;  /* maximum result sequence length */
   int resume;	     /* flag - resumption possible after last result */

   n->postn = postn--;

   switch (n->n_type) {
      case N_Activat:
         /*
          * Activation can fail or succeed.
           */
         arg_life(n, 0L, 1L, 0, 1, 2, resumer, failer, gen);
         break;

      case N_Alt:
         Tree1(n)->lifetime = n->lifetime;
         Tree0(n)->lifetime = n->lifetime;
         liveness(Tree1(n), resumer, &failer2, &gen2);
         liveness(Tree0(n), resumer, &failer1, &gen1);
         *failer = failer2;
         *gen = 1;
         break;

      case N_Apply:
         /* 
          * Assume operation can suspend or fail.
          */
         arg_life(n, 0L, UnbndSeq, 1, 0, 2, resumer, failer, gen);
         break;

      case N_Augop:
         /*
          * Impl0(n) is assignment. Impl1(n) is the augmented operation.
          */
         min_result = Impl0(n)->min_result * Impl1(n)->min_result;
         max_result = Impl0(n)->max_result * Impl1(n)->max_result;
         resume = Impl0(n)->resume | Impl1(n)->resume;
         arg_life(n, min_result, max_result, resume, 2, 2, resumer, failer,
            gen);
         break;

      case N_Bar:
         if (resumer == NULL)
            n->intrnl_lftm = n;
         else
            n->intrnl_lftm = resumer;
         Tree0(n)->lifetime = n->lifetime;
         liveness(Tree0(n), resumer, failer, &gen1);
         *gen = 1;
         break;

      case N_Break:
         if (cur_loop == NULL) {
            nfatal(n, "invalid context for break", NULL);
            return;
            }
         Tree0(n)->lifetime = cur_loop->lifetime;
         loop_sav = cur_loop;
         cur_loop = cur_loop->prev;
         liveness(Tree0(n), loop_sav->resumer, &failer1, &gen1);
         cur_loop = loop_sav;
         cur_loop->gen |= gen1;
         *failer = NULL;
         *gen = 0;
         break;

      case N_Case:
         *failer = resumer;
         *gen = 0;

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
             *  Body.
             */
            Tree1(clause)->lifetime = n->lifetime;
            liveness(Tree1(clause), resumer, &failer2, &gen2);
            if (resumer == NULL && failer2 != NULL)
               *failer = n;
            *gen |= gen2;
      
            /*
             * The expression being compared can be resumed.
             */
            Tree0(clause)->lifetime = clause;
            liveness(Tree0(clause), clause, &failer1, &gen1);
            }

         if (Tree2(n) == NULL) {
            if (resumer == NULL)
               *failer = n;
            }
         else {
            Tree2(n)->lifetime = n->lifetime;
            liveness(Tree2(n), resumer, &failer2, &gen2);  /* default */
            if (resumer == NULL && failer2 != NULL)
               *failer = n;
            *gen |= gen2;
            }

         /*
          * control clause is bounded
          */
         Tree0(n)->lifetime = n;
         liveness(Tree0(n), NULL, &failer1, &gen1);
         if (failer1 != NULL && *failer == NULL)
            *failer = failer1;
         break;

      case N_Create:
         Tree0(n)->lifetime = n;
         loop_sav = cur_loop;
         cur_loop = NULL;        /* check for invalid break and next */
         liveness(Tree0(n), n, &failer1, &gen1);
         cur_loop = loop_sav;
         *failer = NULL;
         *gen = 0;
         break;

      case N_Cset:
      case N_Empty:
      case N_Id: 
      case N_Int:
      case N_Real:
      case N_Str:
         *failer = resumer;
         *gen = 0;
         break;

      case N_Field:
         Tree0(n)->lifetime = n;
         liveness(Tree0(n), resumer, failer, gen);
         break;

      case N_If:
         Tree1(n)->lifetime = n->lifetime;
         liveness(Tree1(n), resumer, failer, gen);
         if (Tree2(n)->n_type != N_Empty) {
            Tree2(n)->lifetime = n->lifetime;
            liveness(Tree2(n), resumer, &failer2, &gen2);
            if (failer2 != NULL) {
               if (*failer == NULL)
                  *failer = failer2;
               else {
                 if ((*failer)->postn < failer2->postn)
                   *failer = failer2;
                 if ((*failer)->postn < n->postn)
                   *failer = n;
                 }
               }
            *gen |= gen2;
            }
         /*
          * control clause is bounded
          */
         Tree0(n)->lifetime = NULL;
         liveness(Tree0(n), NULL, &failer1, &gen1);
         if (Tree2(n)->n_type == N_Empty && failer1 != NULL && *failer == NULL)
            *failer = failer1;
         break;

      case N_Invok:
         /*
          * Assume operation can suspend and fail.
          */
         arg_life(n, 0L, UnbndSeq, 1, 1, Val0(n) + 1, resumer, failer, gen);
         break;

      case N_InvOp:
         arg_life(n, Impl1(n)->min_result, Impl1(n)->max_result,
            Impl1(n)->resume, 2, Val0(n), resumer, failer, gen);
         break;

      case N_InvProc:
         if (Proc1(n)->ret_flag & DoesFail)
             min_result = 0L;
         else
             min_result = 1L;
         if (Proc1(n)->ret_flag & DoesSusp) {
             max_result = UnbndSeq;
             resume = 1;
             }
         else {
             max_result = 1L;
             resume = 0;
             }
         arg_life(n, min_result, max_result, resume, 2, Val0(n), resumer,
            failer, gen);
         break;

      case N_InvRec:
         arg_life(n, err_conv ? 0L : 1L, 1L, 1, 2, Val0(n), resumer, failer,
            gen);
         break;

      case N_Limit:
         if (resumer == NULL)
            n->intrnl_lftm = n;
         else
            n->intrnl_lftm = resumer;
         Tree0(n)->lifetime = n->lifetime;
         liveness(Tree0(n), resumer, &failer1, &gen1);
         Tree1(n)->lifetime = n;
         liveness(Tree1(n), failer1 == NULL ? n : failer1, &failer2, &gen2);
         *failer = failer2;
         *gen = gen1 | gen2;
         break;

      case N_Loop: {
         loop_info.prev = cur_loop;
         loop_info.resumer = resumer;
         loop_info.gen = 0;
         loop_info.every_cntrl = 0;
         loop_info.lifetime = n->lifetime;
         cur_loop = &loop_info;
         switch ((int)Val0(Tree0(n))) {
            case EVERY:
               /*
                * The body is bounded. The control clause is resumed
                *  by the control structure.
                */
               Tree2(n)->lifetime = NULL;
               liveness(Tree2(n), NULL, &failer2, &gen2);
               loop_info.every_cntrl = 1;
               Tree1(n)->lifetime = NULL;
               liveness(Tree1(n), n, &failer1, &gen1);
               break;

            case REPEAT:
               /*
                * The body is bounded.
                */
               Tree1(n)->lifetime = NULL;
               liveness(Tree1(n), NULL, &failer1, &gen1);
               break;

            case SUSPEND:
               /*
                * The body is bounded. The control clause is resumed
                *  by the control structure.
                */
               Tree2(n)->lifetime = NULL;
               liveness(Tree2(n), NULL, &failer2, &gen2);
               loop_info.every_cntrl = 1;
               Tree1(n)->lifetime = n;
               liveness(Tree1(n), n, &failer1, &gen1);
               break;

            case WHILE:
            case UNTIL:
               /*
                * The body and the control clause are each bounded.
                */
               Tree2(n)->lifetime = NULL;
               liveness(Tree2(n), NULL, &failer1, &gen1);
               Tree1(n)->lifetime = NULL;
               liveness(Tree1(n), NULL, &failer1, &gen1);
               break;
            }
         *failer = (resumer == NULL ? n : resumer); /* assume a loop can fail */
         *gen = cur_loop->gen;
         cur_loop = cur_loop->prev;
         }
         break;

      case N_Next:
         if (cur_loop == NULL) {
            nfatal(n, "invalid context for next", NULL);
            return;
            }
         if (cur_loop->every_cntrl)
             *failer = n;
         else
             *failer = NULL;
         *gen = 0;
         break;

      case N_Not:
         /*
          * The expression is bounded.
          */
         Tree0(n)->lifetime = NULL;
         liveness(Tree0(n), NULL, &failer1, &gen1);
         *failer = (resumer == NULL ? n : resumer);
         *gen = 0;
         break;

      case N_Ret:
         if (Val0(Tree0(n)) == RETURN)  {
            /*
             * The expression is bounded.
             */
            Tree1(n)->lifetime = n;
            liveness(Tree1(n), NULL, &failer1, &gen1);
            }
         *failer = NULL;
         *gen = 0;
         break;

      case N_Scan: {
         struct implement *asgn_impl;

         if (resumer == NULL)
            n->intrnl_lftm = n;
         else
            n->intrnl_lftm = resumer;

         if (optab[Val0(Tree0(n))].tok.t_type == AUGQMARK) {
            asgn_impl = optab[asgn_loc].binary;
            arg_life(n, asgn_impl->min_result, asgn_impl->max_result,
               asgn_impl->resume, 1, 2, resumer, failer, gen);
            }
         else {
            Tree2(n)->lifetime = n->lifetime;
            liveness(Tree2(n), resumer, &failer2, &gen2);  /* body */
            Tree1(n)->lifetime = n;
            liveness(Tree1(n), failer2, &failer1, &gen1);  /* subject */ 
            *failer = failer1;
            *gen = gen1 | gen2;
            }
         }
         break;

      case N_Sect:
         /*
          * Impl0(n) is sectioning.
          */
         min_result = Impl0(n)->min_result;
         max_result = Impl0(n)->max_result;
         resume = Impl0(n)->resume;
         if (Impl1(n) != NULL) {
            /*
             * Impl1(n) is plus or minus.
             */
            min_result *= Impl1(n)->min_result;
            max_result *= Impl1(n)->max_result;
            resume |= Impl1(n)->resume;
            }
         arg_life(n, min_result, max_result, resume, 2, 3, resumer, failer,
            gen);
         break;

      case N_Slist:
         /*
          * expr1 is not bounded, expr0 is bounded.
          */
         Tree1(n)->lifetime = n->lifetime;
         liveness(Tree1(n), resumer, failer, gen);
         Tree0(n)->lifetime = NULL;
         liveness(Tree0(n), NULL, &failer1, &gen1);
         break;

      case N_SmplAsgn:
         Tree3(n)->lifetime = n;
         liveness(Tree3(n), resumer, failer, gen);     /* 2nd operand */
         Tree2(n)->lifetime = n->lifetime;             /* may be result of := */
         liveness(Tree2(n), *failer, &failer1, &gen1); /* 1st operand */
         break;

      case N_SmplAug:
         /*
          * Impl1(n) is the augmented operation.
          */
         arg_life(n, Impl1(n)->min_result, Impl1(n)->max_result,
            Impl1(n)->resume, 2, 2, resumer, failer, gen);
         break;

      default:
         fprintf(stderr, "compiler error: node type %d unknown\n", n->n_type);
         exit(EXIT_FAILURE);
      }
   }

/*
 * arg_life - compute the lifetimes of an argument list.
 */
static void arg_life(n, min_result, max_result, resume, frst_arg, nargs,
   resumer, failer, gen)
nodeptr n;
long min_result;  /* minimum result sequence length */
long max_result;  /* maximum result sequence length */
int resume;	  /* flag - resumption possible after last result */
int frst_arg;
int nargs;
nodeptr resumer;
nodeptr *failer;
int *gen;
   {
   nodeptr failer1;
   nodeptr failer2;
   nodeptr lifetime;
   int inv_fail;    /* failure after operation in invoked */
   int reuse;
   int gen2;
   int i;

   /*
    * Determine what, if anything, can resume the rightmost argument.
    */
   if (resumer == NULL && min_result == 0)
      failer1 = n;
   else
      failer1 = resumer;
   if (failer1 == NULL)
      inv_fail = 0;
   else
      inv_fail = 1;

   /*
    * If the operation can be resumed, variables internal to the operation
    *  have and extended lifetime.
    */
   if (resumer != NULL && (max_result > 1 || max_result == UnbndSeq || resume))
      n->intrnl_lftm = resumer;
   else
      n->intrnl_lftm = n;

   /*
    * Go through the parameter list right to left, propagating resumption
    *  information, computing lifetimes, and determining whether anything
    *  can generate.
    */
   lifetime = n;
   reuse = 0;
   *gen = 0;
   for (i = frst_arg + nargs - 1; i >= frst_arg; --i) {
      n->n_field[i].n_ptr->lifetime = lifetime;
      n->n_field[i].n_ptr->reuse = reuse;
      liveness(n->n_field[i].n_ptr, failer1, &failer2, &gen2);
      if (resumer != NULL && gen2)
         lifetime = resumer;
      if (inv_fail && gen2)
         reuse = 1;
      failer1 = failer2;
      *gen |= gen2;
      }
   *failer = failer1;
   if (max_result > 1 || max_result == UnbndSeq)
      *gen = 1;
   }
