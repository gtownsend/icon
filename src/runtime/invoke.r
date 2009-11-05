/*
 * invoke.r -- Perform setup for invocation.
 */

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

      if ((nparam < 0) || (proc->ndynam == -2))
         return I_Vararg;
      else
         return I_Builtin;
      }

   /*
    * Make a stab at catching interpreter stack overflow.  This does
    * nothing for invocation in a co-expression other than &main.
    */
   if (BlkLoc(k_current) == BlkLoc(k_main) &&
      ((char *)sp + PerilDelta) > (char *)stackend)
         fatalerr(301, NULL);

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

   return I_Continue;
}
