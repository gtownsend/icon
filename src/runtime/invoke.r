/*
 * invoke.r - contains invoke, apply
 */

#if COMPILER

/*
 * invoke - perform general invocation on a value.
 */
int invoke(nargs, args, rslt, succ_cont)
int nargs;
dptr args;
dptr rslt;
continuation succ_cont;
   {
   tended struct descrip callee;
   struct b_proc *proc;
   C_integer n;

   /*
    * remove the operation being called from the argument list.
    */
   deref(&args[0], &callee);
   ++args;
   nargs -= 1;

   if (is:proc(callee))
      return (*((struct b_proc *)BlkLoc(callee))->ccode)(nargs, args, rslt,
         succ_cont);
   else if (cnv:C_integer(callee, n)) {
      if (n <= 0)
         n += nargs + 1;
      if (n <= 0 || n > nargs)
         return A_Resume;
      *rslt = args[n - 1];
      return A_Continue;
      }
   else if (cnv:string(callee, callee)) {
      proc = strprc(&callee, (C_integer)nargs);
      if (proc == NULL)
         RunErr(106, &callee);
      return (*(proc)->ccode)(nargs, args, rslt, succ_cont);
      }
   else
      RunErr(106, &callee);
   }


/*
 * apply - implement binary bang. Construct an argument list for
 *   invoke() from the callee and the list it is applied to.
 */
int apply(callee, strct, rslt, succ_cont)
dptr callee;
dptr strct;
dptr rslt;
continuation succ_cont;
   {
   tended struct descrip dstrct;
   struct tend_desc *tnd_args;  /* place to tend arguments to invoke() */
   union block *ep;
   int nargs;
   word i, j;
   word indx;
   int signal;

   deref(strct, &dstrct);

   switch (Type(dstrct)) {

      case T_List: {
         /*
          * Copy the arguments from the list into an tended array of descriptors.
          */
         nargs = BlkLoc(dstrct)->list.size + 1;
         tnd_args = malloc(sizeof(struct tend_desc)
            + (nargs - 1) * sizeof(struct descrip));
         if (tnd_args == NULL)
            RunErr(305, NULL);

         tnd_args->d[0] = *callee;
         indx = 1;
         for (ep = BlkLoc(dstrct)->list.listhead;
#ifdef ListFix
	      BlkType(ep) == T_Lelem;
#else					/* ListFix */
	      ep != NULL;
#endif					/* ListFix */
	      ep = ep->lelem.listnext) {
            for (i = 0; i < ep->lelem.nused; i++) {
               j = ep->lelem.first + i;
               if (j >= ep->lelem.nslots)
                  j -= ep->lelem.nslots;
               tnd_args->d[indx++] = ep->lelem.lslots[j];
               }
            }
         tnd_args->num = nargs;
         tnd_args->previous = tend;
         tend = tnd_args;

         signal = invoke(indx, tnd_args->d, rslt, succ_cont);

         tend = tnd_args->previous;
         free(tnd_args);
         return signal;
         }
      case T_Record: {
         /*
          * Copy the arguments from the record into an tended array
          * of descriptors.
          */
         nargs = BlkLoc(dstrct)->record.recdesc->proc.nfields;
         tnd_args = malloc(sizeof(struct tend_desc)
            + (nargs - 1) * sizeof(struct descrip));
         if (tnd_args == NULL)
            RunErr(305, NULL);

         tnd_args->d[0] = *callee;
         indx = 1;
         ep = BlkLoc(dstrct);
         for (i = 0; i < nargs; i++)
            tnd_args->d[indx++] = ep->record.fields[i];
         tnd_args->num = nargs;
         tnd_args->previous = tend;
         tend = tnd_args;

         signal = invoke(indx, tnd_args->d, rslt, succ_cont);

         tend = tnd_args->previous;
         free(tnd_args);
         return signal;
         }
      default: {
         RunErr(126, &dstrct);
         }
      }
   }

#else					/* COMPILER */

#ifdef EventMon
#include "../h/opdefs.h"
#endif					/* EventMon */


/*
 * invoke -- Perform setup for invocation.
 */
int invoke(nargs,cargp,n)
dptr *cargp;
int nargs, *n;
{
   register struct pf_marker *newpfp;
   register dptr newargp;
   register word *newsp = sp;
   tended struct descrip arg_sv;
   register word i;
   struct b_proc *proc;
   int nparam;

   /*
    * Point newargp at Arg0 and dereference it.
    */
   newargp = (dptr )(sp - 1) - nargs;

   xnargs = nargs;
   xargp = newargp;

   Deref(newargp[0]);

   /*
    * See what course the invocation is to take.
    */
   if (newargp->dword != D_Proc) {
      C_integer tmp;
      /*
       * Arg0 is not a procedure.
       */

      if (cnv:C_integer(newargp[0], tmp)) {
         MakeInt(tmp,&newargp[0]);

         /*
	  * Arg0 is an integer, select result.
	  */
         i = cvpos(IntVal(newargp[0]), (word)nargs);
         if (i == CvtFail || i > nargs)
            return I_Fail;
         newargp[0] = newargp[i];
         sp = (word *)newargp + 1;
         return I_Continue;
         }
      else {
         struct b_proc *tmp;
         /*
	  * See if Arg0 can be converted to a string that names a procedure
	  *  or operator.  If not, generate run-time error 106.
	  */
	 if (!cnv:tmp_string(newargp[0],newargp[0]) ||
	     ((tmp = strprc(newargp, (C_integer)nargs)) == NULL)) {
            err_msg(106, newargp);
            return I_Fail;
            }
	 BlkLoc(newargp[0]) = (union block *)tmp;
	 newargp[0].dword = D_Proc;
	 }
      }

   /*
    * newargp[0] is now a descriptor suitable for invocation.  Dereference
    *  the supplied arguments.
    */

   proc = (struct b_proc *)BlkLoc(newargp[0]);
   if (proc->nstatic >= 0)	/* if negative, don't reference arguments */
      for (i = 1; i <= nargs; i++)
         Deref(newargp[i]);

   /*
    * Adjust the argument list to conform to what the routine being invoked
    *  expects (proc->nparam).  If nparam is less than 0, the number of
    *  arguments is variable. For functions (ndynam = -1) with a
    *  variable number of arguments, nothing need be done.  For Icon procedures
    *  with a variable number of arguments, arguments beyond abs(nparam) are
    *  put in a list which becomes the last argument.  For fix argument
    *  routines, if too many arguments were supplied, adjusting the stack
    *  pointer is all that is necessary. If too few arguments were supplied,
    *  null descriptors are pushed for each missing argument.
    */

   proc = (struct b_proc *)BlkLoc(newargp[0]);
   nparam = (int)proc->nparam;
   if (nparam >= 0) {
      if (nargs > nparam)
         newsp -= (nargs - nparam) * 2;
      else if (nargs < nparam) {
         i = nparam - nargs;
         while (i--) {
            *++newsp = D_Null;
            *++newsp = 0;
            }
         }
      nargs = nparam;

      xnargs = nargs;

      }
   else {
      if (proc->ndynam >= 0) { /* this is a procedure */
         int lelems;
	 dptr llargp;

         if (nargs < abs(nparam) - 1) {
            i = abs(nparam) - 1 - nargs;
            while (i--) {
               *++newsp = D_Null;
               *++newsp = 0;
               }
            nargs = abs(nparam) - 1;
            }

	 lelems = nargs - (abs(nparam) - 1);
         llargp = &newargp[abs(nparam)];
         arg_sv = llargp[-1];

	 Ollist(lelems, &llargp[-1]);

	 llargp[0] = llargp[-1];
	 llargp[-1] = arg_sv;
         /*
          *  Reload proc pointer in case Ollist triggered a garbage collection.
          */
         proc = (struct b_proc *)BlkLoc(newargp[0]);
	 newsp = (word *)llargp + 1;
	 nargs = abs(nparam);
	 }
      }

   if (proc->ndynam < 0) {
      /*
       * A function is being invoked, so nothing else here needs to be done.
       */

      if (nargs < abs(nparam) - 1) {
         i = abs(nparam) - 1 - nargs;
         while (i--) {
            *++newsp = D_Null;
            *++newsp = 0;
            }
         nargs = abs(nparam) - 1;
         }

      *n = nargs;
      *cargp = newargp;
      sp = newsp;

      EVVal((word)Op_Invoke,E_Ecall);

      if ((nparam < 0) || (proc->ndynam == -2))
         return I_Vararg;
      else
         return I_Builtin;
      }

#ifndef MultiThread
   /*
    * Make a stab at catching interpreter stack overflow.  This does
    * nothing for invocation in a co-expression other than &main.
    */
   if (BlkLoc(k_current) == BlkLoc(k_main) &&
      ((char *)sp + PerilDelta) > (char *)stackend)
         fatalerr(301, NULL);
#endif					/* MultiThread */

   /*
    * Build the procedure frame.
    */
   newpfp = (struct pf_marker *)(newsp + 1);
   newpfp->pf_nargs = nargs;
   newpfp->pf_argp = glbl_argp;
   newpfp->pf_pfp = pfp;
   newpfp->pf_ilevel = ilevel;
   newpfp->pf_scan = NULL;

   newpfp->pf_ipc = ipc;
   newpfp->pf_gfp = gfp;
   newpfp->pf_efp = efp;

#ifdef MultiThread
   newpfp->pf_prog = curpstate;
#endif					/* MultiThread */

   glbl_argp = newargp;
   pfp = newpfp;
   newsp += Vwsizeof(*pfp);

   /*
    * If tracing is on, use ctrace to generate a message.
    */
   if (k_trace) {
      k_trace--;
      ctrace(&(proc->pname), nargs, &newargp[1]);
      }

   /*
    * Point ipc at the icode entry point of the procedure being invoked.
    */
   ipc.opnd = (word *)proc->entryp.icode;

#ifdef MultiThread
   /*
    * Enter the program state of the procedure being invoked.
    */
   if (!InRange(code, ipc.opnd, ecode)) {
      syserr("interprogram procedure calls temporarily prohibited\n");
      }
#endif					/* MultiThread */

   efp = 0;
   gfp = 0;

   /*
    * Push a null descriptor on the stack for each dynamic local.
    */
   for (i = proc->ndynam; i > 0; i--) {
      *++newsp = D_Null;
      *++newsp = 0;
      }
   sp = newsp;
   k_level++;

   EVValD(newargp, E_Pcall);

   return I_Continue;
}

#endif					/* COMPILER */
