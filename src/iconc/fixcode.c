/*
 * fixcode.c - routines to "fix code" by determining what signals are returned
 *   by continuations and what must be done when they are.  Also perform
 *   optional control flow optimizations.
 */
#include "../h/gsupport.h"
#include "ctrans.h"
#include "cglobals.h"
#include "ccode.h"
#include "ctree.h"
#include "csym.h"
#include "cproto.h"

/*
 * Prototypes for static functions.
 */
static struct code    *ck_unneed (struct code *cd, struct code *lbl);
static void         clps_brch (struct code *branch);
static void         dec_refs  (struct code *cd);
static void         rm_unrch  (struct code *cd);

/*
 * fix_fncs - go through the generated C functions, determine how calls
 *  handle signals, in-line trivial functions where possible, remove
 *  goto's which immediately precede their labels, and remove unreachable
 *  code.
 */
void fix_fncs(fnc)
struct c_fnc *fnc;
   {
   struct code *cd, *cd1;
   struct code *contbody;
   struct sig_act *sa;
   struct sig_lst *sl;
   struct code *call;
   struct code *create;
   struct code *ret_sig;
   struct code *sig;
   struct c_fnc *calledcont;
   int no_break;
   int collapse;

   /*
    * Fix any called functions and decide how the calls handle the
    *  returned signals.
    */
   fnc->flag |= CF_Mark;
   for (call = fnc->call_lst; call != NULL; call = call->NextCall) {
      calledcont = call->Cont;
      if (calledcont != NULL) {
         if  (!(calledcont->flag & CF_Mark))
            fix_fncs(calledcont);
         if (calledcont->flag & CF_ForeignSig) {
            call->Flags |= ForeignSig;
            fnc->flag |= CF_ForeignSig;
            }
         }


      /*
       * Try to collapse call chains of continuations.
       */
      if (opt_cntrl && calledcont != NULL) {
         contbody = calledcont->cd.next;
         if (call->OperName == NULL && contbody->cd_id == C_RetSig) {
           /*
            * A direct call of a continuation which consists of just a
            *  return. Replace call with code to handle the returned signal.
            */
           ret_sig = contbody->SigRef->sig;
           if (ret_sig == &resume)
               cd1 = sig_cd(call->ContFail, fnc);
            else
               cd1 = sig_cd(ret_sig, fnc);
            cd1->prev = call->prev;
            cd1->prev->next = cd1;
            cd1->next = call->next;
            if (cd1->next != NULL)
               cd1->next->prev = cd1;
            --calledcont->ref_cnt;
            continue;	/* move on to next call */
            }
         else if (contbody->cd_id == C_CallSig && contbody->next == NULL) {
            /*
             * The called continuation contains only a call.
             */
            if (call->OperName == NULL) {
               /*
                * We call the continuation directly, so we can in-line it.
                *  We must replace signal returns with appropriate actions.
                */
               if (--calledcont->ref_cnt != 0 && contbody->Cont != NULL)
                  ++contbody->Cont->ref_cnt;
               call->OperName = contbody->OperName;
               call->ArgLst = contbody->ArgLst;
               call->Cont = contbody->Cont;
               call->Flags = contbody->Flags;
               for (sa = contbody->SigActs; sa != NULL; sa = sa->next) {
                  ret_sig = sa->cd->SigRef->sig;
                  if (ret_sig == &resume)
                     cd1 = sig_cd(call->ContFail, fnc);
                  else
                     cd1 = sig_cd(ret_sig, fnc);
                  call->SigActs = new_sgact(sa->sig, cd1, call->SigActs);
                  }
               continue;	/* move on to next call */
               }
            else if (contbody->OperName == NULL) {
               /*
                * The continuation simply calls another continuation. We can
                *  eliminate the intermediate continuation as long as we can
                *  move signal conversions to the other side of the operation.
                *  The operation only intercepts resume signals.
                */
               collapse = 1;
               for (sa = contbody->SigActs; sa != NULL; sa = sa->next) {
                  ret_sig = sa->cd->SigRef->sig;
                  if (sa->sig != ret_sig && (sa->sig == &resume ||
                      ret_sig == &resume))
                     collapse = 0;
                  }
               if (collapse) {
                  if (--calledcont->ref_cnt != 0 && contbody->Cont != NULL)
                     ++contbody->Cont->ref_cnt;
                  call->Cont = contbody->Cont;
                  for (sa = contbody->SigActs; sa != NULL; sa = sa->next) {
                     ret_sig = sa->cd->SigRef->sig;
                     if (ret_sig != &resume)
                        call->SigActs = new_sgact(sa->sig, sig_cd(ret_sig, fnc),
                           call->SigActs);
                     }
                  continue;	/* move on to next call */
                  }
               }
            }
         }

      /*
       * We didn't do any optimizations. We must still figure out
       *  out how to handle signals returned by the continuation.
       */
      if (calledcont != NULL) {
         for (sl = calledcont->sig_lst; sl != NULL; sl = sl->next) {
            if (sl->ref_cnt > 0) {
               sig = sl->sig;
               /*
                * If an operation is being called, it handles failure from the
                *  continuation.
                */
               if (sig != &resume || call->OperName == NULL) {
                  if (sig == &resume)
                     cd1 = sig_cd(call->ContFail, fnc);
                  else
                     cd1 = sig_cd(sig, fnc);
                  call->SigActs = new_sgact(sig, cd1, call->SigActs);
                  }
               }
            }
         }
      }

   /*
    * fix up the signal handling in the functions implementing co-expressions.
    */
   for (create = fnc->creatlst; create != NULL; create = create->NextCreat)
      fix_fncs(create->Cont);

   if (!opt_cntrl)
      return;  /* control flow optimizations disabled. */
   /*
    * Collapse branch chains and remove unreachable code.
    */
   for (cd = &(fnc->cd); cd != NULL; cd = cd->next) {
      switch (cd->cd_id) {
         case C_CallSig:
            no_break = 1;
            for (sa = cd->SigActs; sa != NULL; sa = sa->next) {
               if (sa->cd->cd_id == C_Break) {
                  switch (cd->next->cd_id) {
                     case C_Goto:
                        sa->cd->cd_id = cd->next->cd_id;
                        sa->cd->Lbl = cd->next->Lbl;
                        ++sa->cd->Lbl->RefCnt;
                        break;
                     case C_RetSig:
                        sa->cd->cd_id = cd->next->cd_id;
                        sa->cd->SigRef= cd->next->SigRef;
                        ++sa->cd->SigRef->ref_cnt;
                        break;
                     default: 
                        no_break = 0;
                     }
                  }
               if (sa->cd->cd_id == C_Goto)
                  clps_brch(sa->cd);
               }
            if (no_break)
               rm_unrch(cd);
            /*
             * Try converting gotos into breaks.
             */
            for (sa = cd->SigActs; sa != NULL; sa = sa->next)
               if (sa->cd->cd_id == C_Goto) {
                  cd1 = cd->next;
                  while (cd1 != NULL && (cd1->cd_id == C_Label ||
                     cd1->cd_id == C_RBrack)) {
                        if (cd1 == sa->cd->Lbl) {
                           sa->cd->cd_id = C_Break;
                           --cd1->RefCnt;
                           break;
                           }
                        cd1 = cd1->next;
                        }
                  }
            break;

         case C_Goto:
            clps_brch(cd);
            rm_unrch(cd);
            if (cd->cd_id == C_Goto)
               ck_unneed(cd, cd->Lbl);
            break;

         case C_If:
            if (cd->ThenStmt->cd_id == C_Goto) {
               clps_brch(cd->ThenStmt);
               if (cd->ThenStmt->cd_id == C_Goto)
                  ck_unneed(cd, cd->ThenStmt->Lbl);
               }
            break;

         case C_PFail:
         case C_PRet:
         case C_RetSig:
            rm_unrch(cd);
            break;
         }
      }

   /*
    * If this function only contains a return, indicate that we can
    *  call a shared signal returning function instead of it. This is
    *  a special case of "common subROUTINE elimination".
    */
   if (fnc->cd.next->cd_id == C_RetSig)
      fnc->flag |= CF_SigOnly;
   }

/*
 * clps_brch - collapse branch chains.
 */
static void clps_brch(branch)
struct code *branch;
   {
   struct code *cd;
   int save_id;

   cd = branch->Lbl->next;
   while (cd->cd_id == C_Label)
      cd = cd->next;

   /*
    * Avoid infinite recursion on empty infinite loops.
    */
   save_id = branch->cd_id;
   branch->cd_id = 0;
   if (cd->cd_id == C_Goto)
      clps_brch(cd);
   branch->cd_id = save_id;

   switch (cd->cd_id) {
      case C_Goto:
         --branch->Lbl->RefCnt;
         ++cd->Lbl->RefCnt;
         branch->Lbl = cd->Lbl;
         break;
      case C_RetSig:
         /*
          * This optimization requires that C_Goto have as many fields
          *  as C_RetSig.
          */
         --branch->Lbl->RefCnt;
         ++cd->SigRef->ref_cnt;
         branch->cd_id = C_RetSig;
         branch->SigRef = cd->SigRef;
         break;
      }
   }

/*
 * rm_unrch - any code after the given point up to the next label is
 *   unreachable. Remove it.
 */
static void rm_unrch(cd)
struct code *cd;
   {
   struct code *cd1;

   for (cd1 = cd->next; cd1 != NULL && cd1->cd_id != C_LBrack &&
      (cd1->cd_id != C_Label || cd1->RefCnt == 0); cd1 = cd1->next) {
         if (cd1->cd_id == C_RBrack) {
            /*
             * Continue deleting past a '}', but don't delete the '}' itself.
             */
            cd->next = cd1;
            cd1->prev = cd;
            cd = cd1;
            }
         else
            dec_refs(cd1);
         }
   cd->next = cd1;
   if (cd1 != NULL)
      cd1->prev = cd;
   }

/*
 * dec_refs - decrement reference counts for things this code references.
 */
static void dec_refs(cd)
struct code *cd;
   {
   struct sig_act *sa;

   if (cd == NULL)
      return;
   switch (cd->cd_id) {
      case C_Goto:
         --cd->Lbl->RefCnt;
         return;
      case C_RetSig:
         --cd->SigRef->ref_cnt;
         return;
      case C_CallSig:
         if (cd->Cont != NULL)
            --cd->Cont->ref_cnt;
         for (sa = cd->SigActs; sa != NULL; sa = sa->next)
            dec_refs(sa->cd);
         return;
     case C_If:
         dec_refs(cd->ThenStmt);
         return;
     case C_Create:
        --cd->Cont->ref_cnt;
         return;
        }
   }

/*
 * ck_unneed - if there is nothing between a goto and its label, except
 *   perhaps other labels or '}', it is useless, so remove it.
 */
static struct code *ck_unneed(cd, lbl)
struct code *cd;
struct code *lbl;
   {
   struct code *cd1;

   cd1 = cd->next;
   while (cd1 != NULL && (cd1->cd_id == C_Label || cd1->cd_id == C_RBrack)) {
      if (cd1 == lbl) {
         cd = cd->prev;
         cd->next = cd->next->next;
         cd->next->prev = cd;
         --lbl->RefCnt;
         break;
         }
      cd1 = cd1->next;
      }
   return cd;
   }

