/*
 * File: imisc.r
 *  Contents: field, mkrec, limit, llist, bscan, escan
 */

/*
 * x.y - access field y of record x.
 */

LibDcl(field,2,".")
   {
   register word fnum;
   register struct b_record *rp;
   register dptr dp;

   extern int *ftabp;
   extern word *records;

   Deref(Arg1);

   /*
    * Arg1 must be a record and Arg2 must be a field number.
    */
   if (!is:record(Arg1))
      RunErr(107, &Arg1);
   if (IntVal(Arg2) == -1)	/* if was known bad at ilink time */
      RunErr(207, &Arg1);	/* was warning then, now it's fatal */

   /*
    * Map the field number into a field number for the record x.
    */
   rp = (struct b_record *) BlkLoc(Arg1);
   fnum = ftabp[IntVal(Arg2) * *records + rp->recdesc->proc.recnum - 1];

   /*
    * If fnum < 0, x doesn't contain the specified field.
    */
   if (fnum < 0)
      RunErr(207, &Arg1);

   /*
    * Return a pointer to the descriptor for the appropriate field.
    */
   dp = &rp->fields[fnum];
   Arg0.dword = D_Var + ((word *)dp - (word *)rp);
   VarLoc(Arg0) = (dptr)rp;
   Return;
   }


/*
 * mkrec - create a record.
 */

LibDcl(mkrec,-1,"mkrec")
   {
   register int i;
   register struct b_proc *bp;
   register struct b_record *rp;

   /*
    * Be sure that call is from a procedure.
    */

   /*
    * Get a pointer to the record constructor procedure and allocate
    *  a record with the appropriate number of fields.
    */
   bp = (struct b_proc *) BlkLoc(Arg0);
   Protect(rp = alcrecd((int)bp->nfields, (union block *)bp), RunErr(0,NULL));

   /*
    * Set all fields in the new record to null value.
    */
   for (i = (int)bp->nfields; i > nargs; i--)
      rp->fields[i-1] = nulldesc;

   /*
    * Assign each argument value to a record element and dereference it.
    */
   for ( ; i > 0; i--) {
      rp->fields[i-1] = cargp[i]; /* Arg(i), expanded to avoid CLCC bug on Sun*/
      Deref(rp->fields[i-1]);
      }

   ArgType(0) = D_Record;
   Arg0.vword.bptr = (union block *)rp;
   Return;
   }

/*
 * limit - explicit limitation initialization.
 */


LibDcl(limit,2,"\\")
   {

   C_integer tmp;

   /*
    * The limit is both passed and returned in Arg0.  The limit must
    *  be an integer.  If the limit is 0, the expression being evaluated
    *  fails.  If the limit is < 0, it is an error.  Note that the
    *  result produced by limit is ultimately picked up by the lsusp
    *  function.
    */
   Deref(Arg0);

   if (!cnv:C_integer(Arg0,tmp))
      RunErr(101, &Arg0);
   MakeInt(tmp,&Arg0);

   if (IntVal(Arg0) < 0)
      RunErr(205, &Arg0);
   if (IntVal(Arg0) == 0)
      Fail;
   Return;
   }

/*
 * bscan - set &subject and &pos upon entry to a scanning expression.
 *
 *  Arguments are:
 *	Arg0 - new value for &subject
 *	Arg1 - saved value of &subject
 *	Arg2 - saved value of &pos
 *
 * A variable pointing to the saved &subject and &pos is returned to be
 *  used by escan.
 */

LibDcl(bscan,2,"?")
   {
   int rc;
   struct pf_marker *cur_pfp;

   /*
    * Convert the new value for &subject to a string.
    */
   Deref(Arg0);

   if (!cnv:string(Arg0,Arg0))
      RunErr(103, &Arg0);

   /*
    * Establish a new &subject value and set &pos to 1.
    */
   k_subject = Arg0;
   k_pos = 1;

   /* If the saved scanning environment belongs to the current procedure
    *  call, put a reference to it in the procedure frame.
    */
   if (pfp->pf_scan == NULL)
      pfp->pf_scan = &Arg1;
   cur_pfp = pfp;

   /*
    * Suspend with a variable pointing to the saved &subject and &pos.
    */
   ArgType(0) = D_Var;
   VarLoc(Arg0) = &Arg1;

   rc = interp(G_Csusp,cargp);

   if (pfp != cur_pfp)
      return rc;

   /*
    * Leaving scanning environment. Restore the old &subject and &pos values.
    */
   k_subject = Arg1;
   k_pos = IntVal(Arg2);

   if (pfp->pf_scan == &Arg1)
      pfp->pf_scan = NULL;

   return rc;

   }

/*
 * escan - restore &subject and &pos at the end of a scanning expression.
 *
 *  Arguments:
 *    Arg0 - variable pointing to old values of &subject and &pos
 *    Arg1 - result of the scanning expression
 *
 * The two arguments are reversed, so that the result of the scanning
 *  expression becomes the result of escan. This result is dereferenced
 *  if it refers to &subject or &pos. Then the saved values of &subject
 *  and &pos are exchanged with the current ones.
 *
 * Escan suspends once it has restored the old &subject; on failure
 *  the new &subject and &pos are "unrestored", and the failure is
 *  propagated into the using clause.
 */

LibDcl(escan,1,"escan")
   {
   struct descrip tmp;
   int rc;
   struct pf_marker *cur_pfp;

   /*
    * Copy the result of the scanning expression into Arg0, which will
    *  be the result of the scan.
    */
   tmp = Arg0;
   Arg0 = Arg1;
   Arg1 = tmp;

   /*
    * If the result of the scanning expression is &subject or &pos,
    *  it is dereferenced. #%#%  following is incorrect #%#%
    */
   /*if ((Arg0 == k_subject) ||
      (Arg0 == kywd_pos))
         Deref(Arg0); */

   /*
    * Swap new and old values of &subject
    */
   tmp = k_subject;
   k_subject = *VarLoc(Arg1);
   *VarLoc(Arg1) = tmp;

   /*
    * Swap new and old values of &pos
    */
   tmp = *(VarLoc(Arg1) + 1);
   IntVal(*(VarLoc(Arg1) + 1)) = k_pos;
   k_pos = IntVal(tmp);

   /*
    * If we are returning to the scanning environment of the current
    *  procedure call, indicate that it is no longed in a saved state.
    */
   if (pfp->pf_scan == VarLoc(Arg1))
      pfp->pf_scan = NULL;
   cur_pfp = pfp;

   /*
    * Suspend with the value of the scanning expression.
    */

   rc = interp(G_Csusp,cargp);
   if (pfp != cur_pfp)
      return rc;

   /*
    * Re-entering scanning environment, exchange the values of &subject
    *  and &pos again
    */
   tmp = k_subject;
   k_subject = *VarLoc(Arg1);
   *VarLoc(Arg1) = tmp;

   tmp = *(VarLoc(Arg1) + 1);
   IntVal(*(VarLoc(Arg1) + 1)) = k_pos;
   k_pos = IntVal(tmp);

   if (pfp->pf_scan == NULL)
      pfp->pf_scan = VarLoc(Arg1);

   return rc;
   }
