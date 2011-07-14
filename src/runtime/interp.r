/*
 * File: interp.r
 *  The interpreter proper.
 */

#include "../h/opdefs.h"

word lastop;			/* Last operator evaluated */

/*
 * Istate variables.
 */
struct ef_marker *efp;		/* Expression frame pointer */
struct gf_marker *gfp;		/* Generator frame pointer */
inst ipc;			/* Interpreter program counter */
word *sp = NULL;		/* Stack pointer */

int ilevel;			/* Depth of recursion in interp() */
struct descrip value_tmp;	/* list argument to Op_Apply */
struct descrip eret_tmp;	/* eret value during unwinding */

int coexp_act;			/* last co-expression action */

dptr xargp;
word xnargs;

/*
 * Macros for use inside the main loop of the interpreter.
 */

/*
 * Setup_Op sets things up for a call to the C function for an operator.
 */
#begdef Setup_Op(nargs)
   rargp = (dptr)(rsp - 1) - nargs;
   xargp = rargp;
   ExInterp;
#enddef					/* Setup_Op */

/*
 * Setup_Arg sets things up for a call to the C function.
 *  It is the same as Setup_Op, except the latter is used only
 *  operators.
 */
#begdef Setup_Arg(nargs)
   rargp = (dptr)(rsp - 1) - nargs;
   xargp = rargp;
   ExInterp;
#enddef					/* Setup_Arg */

#begdef Call_Cond
   if ((*(optab[lastop]))(rargp) == A_Resume) {
     goto efail_noev;
   }
   rsp = (word *) rargp + 1;
   break;
#enddef					/* Call_Cond */

/*
 * Call_Gen - Call a generator. A C routine associated with the
 *  current opcode is called. When it when it terminates, control is
 *  passed to C_rtn_term to deal with the termination condition appropriately.
 */
#begdef Call_Gen
   signal = (*(optab[lastop]))(rargp);
   goto C_rtn_term;
#enddef					/* Call_Gen */

/*
 * GetWord fetches the next icode word.  PutWord(x) stores x at the current
 * icode word.
 */
#define GetWord (*ipc.opnd++)
#define PutWord(x) ipc.opnd[-1] = (x)
#define GetOp (word)(*ipc.op++)
#define PutOp(x) ipc.op[-1] = (x)

/*
 * DerefArg(n) dereferences the nth argument.
 */
#define DerefArg(n)   Deref(rargp[n])

/*
 * For the sake of efficiency, the stack pointer is kept in a register
 *  variable, rsp, in the interpreter loop.  Since this variable is
 *  only accessible inside the loop, and the global variable sp is used
 *  for the stack pointer elsewhere, rsp must be stored into sp when
 *  the context of the loop is left and conversely, rsp must be loaded
 *  from sp when the loop is reentered.  The macros ExInterp and EntInterp,
 *  respectively, handle these operations.  Currently, this register/global
 *  scheme is only used for the stack pointer, but it can be easily extended
 *  to other variables.
 */

#define ExInterp	sp = rsp;
#define EntInterp	rsp = sp;

/*
 * Inside the interpreter loop, PushDesc, PushNull, PushAVal, and
 *  PushVal use rsp instead of sp for efficiency.
 */
#undef PushDesc
#undef PushNull
#undef PushVal
#undef PushAVal
#define PushDesc(d)   PushDescSP(rsp,d)
#define PushNull      PushNullSP(rsp)
#define PushVal(v)    PushValSP(rsp,v)
#define PushAVal(a)   PushValSP(rsp,a)


/*
 * The main loop of the interpreter.
 */
int interp(fsig,cargp)
int fsig;
dptr cargp;
   {
   register word opnd;
   register word *rsp;
   register dptr rargp;
   register struct ef_marker *newefp;
   register struct gf_marker *newgfp;
   register word *wd;
   register word *firstwd, *lastwd;
   word *oldsp;
   int type, signal, args;
   extern int (*optab[])();
   extern int (*keytab[])();
   struct b_proc *bproc;

   /*
    * Make a stab at catching interpreter stack overflow.  This does
    * nothing for invocation in a co-expression other than &main.
    */
   if (BlkLoc(k_current) == BlkLoc(k_main) &&
      ((char *)sp + PerilDelta) > (char *)stackend)
         fatalerr(301, NULL);

#ifdef Graphics
   if (!pollctr--) {
      pollctr = pollevent();
      if (pollctr == -1) fatalerr(141, NULL);
      }
#endif					/* Graphics */

   ilevel++;

   EntInterp;

   if (fsig == G_Csusp) {

      oldsp = rsp;

      /*
       * Create the generator frame.
       */
      newgfp = (struct gf_marker *)(rsp + 1);
      newgfp->gf_gentype = fsig;
      newgfp->gf_gfp = gfp;
      newgfp->gf_efp = efp;
      newgfp->gf_ipc = ipc;
      rsp += Wsizeof(struct gf_smallmarker);

      /*
       * Region extends from first word after the marker for the generator
       *  or expression frame enclosing the call to the now-suspending
       *  routine to the first argument of the routine.
       */
      if (gfp != 0) {
	 if (gfp->gf_gentype == G_Psusp)
	    firstwd = (word *)gfp + Wsizeof(*gfp);
	 else
	    firstwd = (word *)gfp + Wsizeof(struct gf_smallmarker);
	 }
      else
	 firstwd = (word *)efp + Wsizeof(*efp);
      lastwd = (word *)cargp + 1;

      /*
       * Copy the portion of the stack with endpoints firstwd and lastwd
       *  (inclusive) to the top of the stack.
       */
      for (wd = firstwd; wd <= lastwd; wd++)
	 *++rsp = *wd;
      gfp = newgfp;
      }
/*
 * Top of the interpreter loop.
 */

   for (;;) {
      lastop = GetOp;		/* Instruction fetch */
      switch ((int)lastop) {		/*
				 * Switch on opcode.  The cases are
				 * organized roughly by functionality
				 * to make it easier to find things.
				 * For some C compilers, there may be
				 * an advantage to arranging them by
				 * likelihood of selection.
				 */

				/* ---Constant construction--- */

	 case Op_Cset:		/* cset */
	    PutOp(Op_Acset);
	    PushVal(D_Cset);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    PutWord(opnd);
	    PushAVal(opnd);
	    break;

	 case Op_Acset:		/* cset, absolute address */
	    PushVal(D_Cset);
	    PushAVal(GetWord);
	    break;

	 case Op_Int:		/* integer */
	    PushVal(D_Integer);
	    PushVal(GetWord);
	    break;

	 case Op_Real:		/* real */
	    PutOp(Op_Areal);
	    PushVal(D_Real);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    PushAVal(opnd);
	    PutWord(opnd);
	    break;

	 case Op_Areal:		/* real, absolute address */
	    PushVal(D_Real);
	    PushAVal(GetWord);
	    break;

	 case Op_Str:		/* string */
	    PutOp(Op_Astr);
	    PushVal(GetWord);
	    opnd = (word)strcons + GetWord;
	    PutWord(opnd);
	    PushAVal(opnd);
	    break;

	 case Op_Astr:		/* string, absolute address */
	    PushVal(GetWord);
	    PushAVal(GetWord);
	    break;

				/* ---Variable construction--- */

	 case Op_Arg:		/* argument */
	    PushVal(D_Var);
	    PushAVal(&glbl_argp[GetWord + 1]);
	    break;

	 case Op_Global:	/* global */
	    PutOp(Op_Aglobal);
	    PushVal(D_Var);
	    opnd = GetWord;
	    PushAVal(&globals[opnd]);
	    PutWord((word)&globals[opnd]);
	    break;

	 case Op_Aglobal:	/* global, absolute address */
	    PushVal(D_Var);
	    PushAVal(GetWord);
	    break;

	 case Op_Local:		/* local */
	    PushVal(D_Var);
	    PushAVal(&pfp->pf_locals[GetWord]);
	    break;

	 case Op_Static:	/* static */
	    PutOp(Op_Astatic);
	    PushVal(D_Var);
	    opnd = GetWord;
	    PushAVal(&statics[opnd]);
	    PutWord((word)&statics[opnd]);
	    break;

	 case Op_Astatic:	/* static, absolute address */
	    PushVal(D_Var);
	    PushAVal(GetWord);
	    break;


				/* ---Operators--- */

				/* Unary operators */

	 case Op_Compl:		/* ~e */
	 case Op_Neg:		/* -e */
	 case Op_Number:	/* +e */
	 case Op_Refresh:	/* ^e */
	 case Op_Size:		/* *e */
	    Setup_Op(1);
	    DerefArg(1);
	    Call_Cond;

	 case Op_Value:		/* .e */
            Setup_Op(1);
            DerefArg(1);
            Call_Cond;

	 case Op_Nonnull:	/* \e */
	 case Op_Null:		/* /e */
	    Setup_Op(1);
	    Call_Cond;

	 case Op_Random:	/* ?e */
	    PushNull;
	    Setup_Op(2)
	    Call_Cond

				/* Generative unary operators */

	 case Op_Tabmat:	/* =e */
	    Setup_Op(1);
	    DerefArg(1);
	    Call_Gen;

	 case Op_Bang:		/* !e */
	    PushNull;
	    Setup_Op(2);
	    Call_Gen;

				/* Binary operators */

	 case Op_Cat:		/* e1 || e2 */
	 case Op_Diff:		/* e1 -- e2 */
	 case Op_Div:		/* e1 / e2 */
	 case Op_Inter:		/* e1 ** e2 */
	 case Op_Lconcat:	/* e1 ||| e2 */
	 case Op_Minus:		/* e1 - e2 */
	 case Op_Mod:		/* e1 % e2 */
	 case Op_Mult:		/* e1 * e2 */
	 case Op_Power:		/* e1 ^ e2 */
	 case Op_Unions:	/* e1 ++ e2 */
	 case Op_Plus:		/* e1 + e2 */
	 case Op_Eqv:		/* e1 === e2 */
	 case Op_Lexeq:		/* e1 == e2 */
	 case Op_Lexge:		/* e1 >>= e2 */
	 case Op_Lexgt:		/* e1 >> e2 */
	 case Op_Lexle:		/* e1 <<= e2 */
	 case Op_Lexlt:		/* e1 << e2 */
	 case Op_Lexne:		/* e1 ~== e2 */
	 case Op_Neqv:		/* e1 ~=== e2 */
	 case Op_Numeq:		/* e1 = e2 */
	 case Op_Numge:		/* e1 >= e2 */
	 case Op_Numgt:		/* e1 > e2 */
	 case Op_Numle:		/* e1 <= e2 */
	 case Op_Numne:		/* e1 ~= e2 */
	 case Op_Numlt:		/* e1 < e2 */
	    Setup_Op(2);
	    DerefArg(1);
	    DerefArg(2);
	    Call_Cond;

	 case Op_Asgn:		/* e1 := e2 */
	    Setup_Op(2);
	    Call_Cond;

	 case Op_Swap:		/* e1 :=: e2 */
	    PushNull;
	    Setup_Op(3);
	    Call_Cond;

	 case Op_Subsc:		/* e1[e2] */
	    PushNull;
	    Setup_Op(3);
	    Call_Cond;
				/* Generative binary operators */

	 case Op_Rasgn:		/* e1 <- e2 */
	    Setup_Op(2);
	    Call_Gen;

	 case Op_Rswap:		/* e1 <-> e2 */
	    PushNull;
	    Setup_Op(3);
	    Call_Gen;

				/* Conditional ternary operators */

	 case Op_Sect:		/* e1[e2:e3] */
	    PushNull;
	    Setup_Op(4);
	    Call_Cond;
				/* Generative ternary operators */

	 case Op_Toby:		/* e1 to e2 by e3 */
	    Setup_Op(3);
	    DerefArg(1);
	    DerefArg(2);
	    DerefArg(3);
	    Call_Gen;

         case Op_Noop:		/* no-op, used for padding on some configns */
            break;

         case Op_Colm:		/* source column number; no longer used */
            break;

         case Op_Line:		/* source line number; only seen if enabled */
	    if (profiling_active)	
	       countline(ipc.opnd);	/* note this line if we're profiling */
            break;

				/* ---String Scanning--- */

	 case Op_Bscan:		/* prepare for scanning */
	    PushDesc(k_subject);
	    PushVal(D_Integer);
	    PushVal(k_pos);
	    Setup_Arg(2);

	    signal = Obscan(2,rargp);

	    goto C_rtn_term;

	 case Op_Escan:		/* exit from scanning */
	    Setup_Arg(1);

	    signal = Oescan(1,rargp);

	    goto C_rtn_term;

				/* ---Other Language Operations--- */

         case Op_Apply: {	/* apply */
            union block *bp;
            int i, j;

            value_tmp = *(dptr)(rsp - 1);	/* argument */
            Deref(value_tmp);
            switch (Type(value_tmp)) {
               case T_List: {
                  rsp -= 2;				/* pop it off */
                  bp = BlkLoc(value_tmp);
                  args = (int)bp->list.size;

                 /*
                  * Make a stab at catching interpreter stack overflow.
                  * This does nothing for invocation in a co-expression other
                  * than &main.
                  */
                 if (BlkLoc(k_current) == BlkLoc(k_main) &&
                    ((char *)sp + args * sizeof(struct descrip) >
                       (char *)stackend))
                          fatalerr(301, NULL);

                  for (bp = bp->list.listhead;
		     bp != NULL;
                     bp = bp->lelem.listnext) {
                        for (i = 0; i < bp->lelem.nused; i++) {
                           j = bp->lelem.first + i;
                           if (j >= bp->lelem.nslots)
                              j -= bp->lelem.nslots;
                           PushDesc(bp->lelem.lslots[j]);
                           }
                        }
		  goto invokej;
		  }

               case T_Record: {
                  rsp -= 2;		/* pop it off */
                  bp = BlkLoc(value_tmp);
                  args = bp->record.recdesc->proc.nfields;
                  for (i = 0; i < args; i++) {
                     PushDesc(bp->record.fields[i]);
                     }
                  goto invokej;
                  }

               default: {		/* illegal type for invocation */

                  xargp = (dptr)(rsp - 3);
                  err_msg(126, &value_tmp);
                  goto efail;
                  }
               }
	    }

	 case Op_Invoke: {	/* invoke */
            args = (int)GetWord;
invokej:
	    {
            int nargs;
	    dptr carg;

	    ExInterp;
	    type = invoke(args, &carg, &nargs);
	    EntInterp;

	    if (type == I_Fail)
	       goto efail_noev;
	    if (type == I_Continue)
	       break;
	    else {

               rargp = carg;		/* valid only for Vararg or Builtin */

#ifdef Graphics
	       /*
		* Do polling here
		*/
	       pollctr >>= 1;
               if (!pollctr) {
	          ExInterp;
                  pollctr = pollevent();
	          EntInterp;
	          if (pollctr == -1) fatalerr(141, NULL);
	          }
#endif					/* Graphics */

	       bproc = (struct b_proc *)BlkLoc(*rargp);

	       /* ExInterp not needed since no change since last EntInterp */
	       if (type == I_Vararg) {
	          int (*bfunc)();
                  bfunc = bproc->entryp.ccode;
		  signal = (*bfunc)(nargs,rargp);
                  }
	       else
                  {
                  int (*bfunc)();
                  bfunc = bproc->entryp.ccode;
		  signal = (*bfunc)(rargp);
                  }
	       goto C_rtn_term;
	       }
	    }
	    }

	 case Op_Keywd:		/* keyword */

            PushNull;
            opnd = GetWord;
            Setup_Arg(0);

	    signal = (*(keytab[(int)opnd]))(rargp);
	    goto C_rtn_term;

	 case Op_Llist:		/* construct list */
	    opnd = GetWord;
	    Setup_Arg(opnd);
	    {
	    int i;
	    for (i=1;i<=opnd;i++)
               DerefArg(i);
	    }

	    signal = Ollist((int)opnd,rargp);

	    goto C_rtn_term;

				/* ---Marking and Unmarking--- */

	 case Op_Mark:		/* create expression frame marker */
	    PutOp(Op_Amark);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    PutWord(opnd);
	    newefp = (struct ef_marker *)(rsp + 1);
	    newefp->ef_failure.opnd = (word *)opnd;
	    goto mark;

	 case Op_Amark:		/* mark with absolute fipc */
	    newefp = (struct ef_marker *)(rsp + 1);
	    newefp->ef_failure.opnd = (word *)GetWord;
mark:
	    newefp->ef_gfp = gfp;
	    newefp->ef_efp = efp;
	    newefp->ef_ilevel = ilevel;
	    rsp += Wsizeof(*efp);
	    efp = newefp;
	    gfp = 0;
	    break;

	 case Op_Mark0:		/* create expression frame with 0 ipl */
mark0:
	    newefp = (struct ef_marker *)(rsp + 1);
	    newefp->ef_failure.opnd = 0;
	    newefp->ef_gfp = gfp;
	    newefp->ef_efp = efp;
	    newefp->ef_ilevel = ilevel;
	    rsp += Wsizeof(*efp);
	    efp = newefp;
	    gfp = 0;
	    break;

	 case Op_Unmark:	/* remove expression frame */
	    gfp = efp->ef_gfp;
	    rsp = (word *)efp - 1;

	    /*
	     * Remove any suspended C generators.
	     */
Unmark_uw:
	    if (efp->ef_ilevel < ilevel) {
	       --ilevel;

	       ExInterp;
	       return A_Unmark_uw;
	       }

	    efp = efp->ef_efp;
	    break;

				/* ---Suspensions--- */

	 case Op_Esusp: {	/* suspend from expression */

	    /*
	     * Create the generator frame.
	     */
	    oldsp = rsp;
	    newgfp = (struct gf_marker *)(rsp + 1);
	    newgfp->gf_gentype = G_Esusp;
	    newgfp->gf_gfp = gfp;
	    newgfp->gf_efp = efp;
	    newgfp->gf_ipc = ipc;
	    gfp = newgfp;
	    rsp += Wsizeof(struct gf_smallmarker);

	    /*
	     * Region extends from first word after enclosing generator or
	     *	expression frame marker to marker for current expression frame.
	     */
	    if (efp->ef_gfp != 0) {
	       newgfp = (struct gf_marker *)(efp->ef_gfp);
	       if (newgfp->gf_gentype == G_Psusp)
		  firstwd = (word *)efp->ef_gfp + Wsizeof(*gfp);
	       else
		  firstwd = (word *)efp->ef_gfp +
		     Wsizeof(struct gf_smallmarker);
		}
	    else
	       firstwd = (word *)efp->ef_efp + Wsizeof(*efp);
	    lastwd = (word *)efp - 1;
	    efp = efp->ef_efp;

	    /*
	     * Copy the portion of the stack with endpoints firstwd and lastwd
	     *	(inclusive) to the top of the stack.
	     */
	    for (wd = firstwd; wd <= lastwd; wd++)
	       *++rsp = *wd;
	    PushVal(oldsp[-1]);
	    PushVal(oldsp[0]);
	    break;
	    }

	 case Op_Lsusp: {	/* suspend from limitation */
	    struct descrip sval;

	    /*
	     * The limit counter is contained in the descriptor immediately
	     *	prior to the current expression frame.	lval is established
	     *	as a pointer to this descriptor.
	     */
	    dptr lval = (dptr)((word *)efp - 2);

	    /*
	     * Decrement the limit counter and check it.
	     */
	    if (--IntVal(*lval) > 0) {
	       /*
		* The limit has not been reached, set up stack.
		*/

	       sval = *(dptr)(rsp - 1);	/* save result */

	       /*
		* Region extends from first word after enclosing generator or
		*  expression frame marker to the limit counter just prior to
		*  to the current expression frame marker.
		*/
	       if (efp->ef_gfp != 0) {
		  newgfp = (struct gf_marker *)(efp->ef_gfp);
		  if (newgfp->gf_gentype == G_Psusp)
		     firstwd = (word *)efp->ef_gfp + Wsizeof(*gfp);
		  else
		     firstwd = (word *)efp->ef_gfp +
			Wsizeof(struct gf_smallmarker);
		  }
	       else
		  firstwd = (word *)efp->ef_efp + Wsizeof(*efp);
	       lastwd = (word *)efp - 3;
	       if (gfp == 0)
		  gfp = efp->ef_gfp;
	       efp = efp->ef_efp;

	       /*
		* Copy the portion of the stack with endpoints firstwd and lastwd
		*  (inclusive) to the top of the stack.
		*/
	       rsp -= 2;		/* overwrite result */
	       for (wd = firstwd; wd <= lastwd; wd++)
		  *++rsp = *wd;
	       PushDesc(sval);		/* push saved result */
	       }
	    else {
	       /*
		* Otherwise, the limit has been reached.  Instead of
		*  suspending, remove the current expression frame and
		*  replace the limit counter with the value on top of
		*  the stack (which would have been suspended had the
		*  limit not been reached).
		*/
	       *lval = *(dptr)(rsp - 1);
	       gfp = efp->ef_gfp;

	       /*
		* Since an expression frame is being removed, inactive
		*  C generators contained therein are deactivated.
		*/
Lsusp_uw:
	       if (efp->ef_ilevel < ilevel) {
		  --ilevel;
		  ExInterp;
		  return A_Lsusp_uw;
		  }
	       rsp = (word *)efp - 1;
	       efp = efp->ef_efp;
	       }
	    break;
	    }

	 case Op_Psusp: {	/* suspend from procedure */

	    /*
	     * An Icon procedure is suspending a value.  Determine if the
	     *	value being suspended should be dereferenced and if so,
	     *	dereference it. If tracing is on, strace is called
	     *  to generate a message.  Appropriate values are
	     *	restored from the procedure frame of the suspending procedure.
	     */

	    struct descrip tmp;
            dptr svalp;
	    struct b_proc *sproc;
	    svalp = (dptr)(rsp - 1);
	    if (Var(*svalp)) {
               ExInterp;
               retderef(svalp, (word *)glbl_argp, sp);
               EntInterp;
               }

	    /*
	     * Create the generator frame.
	     */
	    oldsp = rsp;
	    newgfp = (struct gf_marker *)(rsp + 1);
	    newgfp->gf_gentype = G_Psusp;
	    newgfp->gf_gfp = gfp;
	    newgfp->gf_efp = efp;
	    newgfp->gf_ipc = ipc;
	    newgfp->gf_argp = glbl_argp;
	    newgfp->gf_pfp = pfp;
	    gfp = newgfp;
	    rsp += Wsizeof(*gfp);

	    /*
	     * Region extends from first word after the marker for the
	     *	generator or expression frame enclosing the call to the
	     *	now-suspending procedure to Arg0 of the procedure.
	     */
	    if (pfp->pf_gfp != 0) {
	       newgfp = (struct gf_marker *)(pfp->pf_gfp);
	       if (newgfp->gf_gentype == G_Psusp)
		  firstwd = (word *)pfp->pf_gfp + Wsizeof(*gfp);
	       else
		  firstwd = (word *)pfp->pf_gfp +
		     Wsizeof(struct gf_smallmarker);
	       }
	    else
	       firstwd = (word *)pfp->pf_efp + Wsizeof(*efp);
	    lastwd = (word *)glbl_argp - 1;
	       efp = efp->ef_efp;

	    /*
	     * Copy the portion of the stack with endpoints firstwd and lastwd
	     *	(inclusive) to the top of the stack.
	     */
	    for (wd = firstwd; wd <= lastwd; wd++)
	       *++rsp = *wd;
	    PushVal(oldsp[-1]);
	    PushVal(oldsp[0]);
	    --k_level;
	    if (k_trace) {
               k_trace--;
	       sproc = (struct b_proc *)BlkLoc(*glbl_argp);
	       strace(&(sproc->pname), svalp);
	       }

	    /*
	     * If the scanning environment for this procedure call is in
	     *	a saved state, switch environments.
	     */
	    if (pfp->pf_scan != NULL) {
	       tmp = k_subject;
	       k_subject = *pfp->pf_scan;
	       *pfp->pf_scan = tmp;

	       tmp = *(pfp->pf_scan + 1);
	       IntVal(*(pfp->pf_scan + 1)) = k_pos;
	       k_pos = IntVal(tmp);
	       }

	    efp = pfp->pf_efp;
	    ipc = pfp->pf_ipc;
	    glbl_argp = pfp->pf_argp;
	    pfp = pfp->pf_pfp;
	    break;
	    }

				/* ---Returns--- */

	 case Op_Eret: {	/* return from expression */
	    /*
	     * Op_Eret removes the current expression frame, leaving the
	     *	original top of stack value on top.
	     */
	    /*
	     * Save current top of stack value in global temporary (no
	     *	danger of reentry).
	     */
	    eret_tmp = *(dptr)&rsp[-1];
	    gfp = efp->ef_gfp;
Eret_uw:
	    /*
	     * Since an expression frame is being removed, inactive
	     *	C generators contained therein are deactivated.
	     */
	    if (efp->ef_ilevel < ilevel) {
	       --ilevel;
	       ExInterp;
	       return A_Eret_uw;
	       }
	    rsp = (word *)efp - 1;
	    efp = efp->ef_efp;
	    PushDesc(eret_tmp);
	    break;
	    }


	 case Op_Pret: {	/* return from procedure */
	    /*
	     * An Icon procedure is returning a value.	Determine if the
	     *	value being returned should be dereferenced and if so,
	     *	dereference it.  If tracing is on, rtrace is called to
	     *	generate a message.  Inactive generators created after
	     *	the activation of the procedure are deactivated.  Appropriate
	     *	values are restored from the procedure frame.
	     */
	    struct b_proc *rproc;
	    rproc = (struct b_proc *)BlkLoc(*glbl_argp);
	    *glbl_argp = *(dptr)(rsp - 1);
	    if (Var(*glbl_argp)) {
               ExInterp;
               retderef(glbl_argp, (word *)glbl_argp, sp);
               EntInterp;
               }

	    --k_level;
	    if (k_trace) {
               k_trace--;
	       rtrace(&(rproc->pname), glbl_argp);
               }
Pret_uw:
	    if (pfp->pf_ilevel < ilevel) {
	       --ilevel;
	       ExInterp;
	       return A_Pret_uw;
	       }

	    rsp = (word *)glbl_argp + 1;
	    efp = pfp->pf_efp;
	    gfp = pfp->pf_gfp;
	    ipc = pfp->pf_ipc;
	    glbl_argp = pfp->pf_argp;
	    pfp = pfp->pf_pfp;

	    break;
	    }

				/* ---Failures--- */

	 case Op_Efail:
efail:
efail_noev:
	    /*
	     * Failure has occurred in the current expression frame.
	     */
	    if (gfp == 0) {
	       /*
		* There are no suspended generators to resume.
		*  Remove the current expression frame, restoring
		*  values.
		*
		* If the failure ipc is 0, propagate failure to the
		*  enclosing frame by branching back to efail.
		*  This happens, for example, in looping control
		*  structures that fail when complete.
		*/

	       ipc = efp->ef_failure;
	       gfp = efp->ef_gfp;
	       rsp = (word *)efp - 1;
	       efp = efp->ef_efp;

	       if (ipc.op == 0)
		  goto efail;
	       break;
	       }

	    else {
	       /*
		* There is a generator that can be resumed.  Make
		*  the stack adjustments and then switch on the
		*  type of the generator frame marker.
		*/
	       struct descrip tmp;
	       register struct gf_marker *resgfp = gfp;

	       type = (int)resgfp->gf_gentype;

	       if (type == G_Psusp) {
		  glbl_argp = resgfp->gf_argp;
		  if (k_trace) {	/* procedure tracing */
                     k_trace--;
		     ExInterp;
		     atrace(&(((struct b_proc *)BlkLoc(*glbl_argp))->pname));
		     EntInterp;
		     }
		  }
	       ipc = resgfp->gf_ipc;
	       efp = resgfp->gf_efp;
	       gfp = resgfp->gf_gfp;
	       rsp = (word *)resgfp - 1;
	       if (type == G_Psusp) {
		  pfp = resgfp->gf_pfp;

		  /*
		   * If the scanning environment for this procedure call is
		   *  supposed to be in a saved state, switch environments.
		   */
		  if (pfp->pf_scan != NULL) {
		     tmp = k_subject;
		     k_subject = *pfp->pf_scan;
		     *pfp->pf_scan = tmp;

		     tmp = *(pfp->pf_scan + 1);
		     IntVal(*(pfp->pf_scan + 1)) = k_pos;
		     k_pos = IntVal(tmp);
		     }

		  ++k_level;		/* adjust procedure level */
		  }

	       switch (type) {

		  case G_Csusp:
		     --ilevel;
		     ExInterp;
		     return A_Resume;

		  case G_Esusp:
		     goto efail_noev;

		  case G_Psusp:		/* resuming a procedure */
		     break;
		  }

	       break;
	       }

	 case Op_Pfail: {	/* fail from procedure */
	    /*
	     * An Icon procedure is failing.  Generate tracing message if
	     *	tracing is on.	Deactivate inactive C generators created
	     *	after activation of the procedure.  Appropriate values
	     *	are restored from the procedure frame.
	     */

	    --k_level;
	    if (k_trace) {
               k_trace--;
	       failtrace(&(((struct b_proc *)BlkLoc(*glbl_argp))->pname));
               }
Pfail_uw:

	    if (pfp->pf_ilevel < ilevel) {
	       --ilevel;
	       ExInterp;
	       return A_Pfail_uw;
	       }
	    efp = pfp->pf_efp;
	    gfp = pfp->pf_gfp;
	    ipc = pfp->pf_ipc;
	    glbl_argp = pfp->pf_argp;
	    pfp = pfp->pf_pfp;
	    goto efail_noev;
	    }
				/* ---Odds and Ends--- */

	 case Op_Ccase:		/* case clause */
	    PushNull;
	    PushVal(((word *)efp)[-2]);
	    PushVal(((word *)efp)[-1]);
	    break;

	 case Op_Chfail:	/* change failure ipc */
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    efp->ef_failure.opnd = (word *)opnd;
	    break;

	 case Op_Dup:		/* duplicate descriptor */
	    PushNull;
	    rsp[1] = rsp[-3];
	    rsp[2] = rsp[-2];
	    rsp += 2;
	    break;

	 case Op_Field:		/* e1.e2 */
	    PushVal(D_Integer);
	    PushVal(GetWord);
	    Setup_Arg(2);

	    signal = Ofield(2,rargp);

	    goto C_rtn_term;

	 case Op_Goto:		/* goto */
	    PutOp(Op_Agoto);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    PutWord(opnd);
	    ipc.opnd = (word *)opnd;
	    break;

	 case Op_Agoto:		/* goto absolute address */
	    opnd = GetWord;
	    ipc.opnd = (word *)opnd;
	    break;

	 case Op_Init:		/* initial */
	    *--ipc.op = Op_Goto;
	    opnd = sizeof(*ipc.op) + sizeof(*rsp);
	    opnd += (word)ipc.opnd;
	    ipc.opnd = (word *)opnd;
	    break;

	 case Op_Limit:		/* limit */
	    Setup_Arg(0);

	    if (Olimit(0,rargp) == A_Resume) {

	       /*
		* limit has failed here; could generate an event for it,
		*  but not an Ofail since limit is not an operator and
		*  no Ocall was ever generated for it.
		*/
	       goto efail_noev;
	       }
	    else {
	       /*
		* limit has returned here; could generate an event for it,
		*  but not an Oret since limit is not an operator and
		*  no Ocall was ever generated for it.
		*/
	       rsp = (word *) rargp + 1;
	       }
	    goto mark0;

	 case Op_Pnull:		/* push null descriptor */
	    PushNull;
	    break;

	 case Op_Pop:		/* pop descriptor */
	    rsp -= 2;
	    break;

	 case Op_Push1:		/* push integer 1 */
	    PushVal(D_Integer);
	    PushVal(1);
	    break;

	 case Op_Pushn1:	/* push integer -1 */
	    PushVal(D_Integer);
	    PushVal(-1);
	    break;

	 case Op_Sdup:		/* duplicate descriptor */
	    rsp += 2;
	    rsp[-1] = rsp[-3];
	    rsp[0] = rsp[-2];
	    break;

					/* ---Co-expressions--- */

	 case Op_Create:	/* create */
	    PushNull;
	    Setup_Arg(0);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;
	    signal = Ocreate((word *)opnd, rargp);
	    goto C_rtn_term;

	 case Op_Coact: {	/* @e */
            struct b_coexpr *ncp;
            dptr dp;

            ExInterp;
            dp = (dptr)(sp - 1);
            xargp = dp - 2;

            Deref(*dp);
            if (dp->dword != D_Coexpr) {
               err_msg(118, dp);
               goto efail;
               }

            ncp = (struct b_coexpr *)BlkLoc(*dp);

            signal = activate((dptr)(sp - 3), ncp, (dptr)(sp - 3));
            EntInterp;
            if (signal == A_Resume)
               goto efail_noev;
            else
               rsp -= 2;
            break;
	    }

	 case Op_Coret: {	/* return from co-expression */
            struct b_coexpr *ncp;

            ExInterp;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            ++BlkLoc(k_current)->coexpr.size;
            co_chng(ncp, (dptr)&sp[-1], NULL, A_Coret, 1);
            EntInterp;
            break;

	    }

	 case Op_Cofail: {	/* fail from co-expression */
            struct b_coexpr *ncp;

            ExInterp;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            co_chng(ncp, NULL, NULL, A_Cofail, 1);
            EntInterp;
            break;

	    }
         case Op_Quit:		/* quit */


	    goto interp_quit;


	 default: {
	    char buf[50];

	    sprintf(buf, "unimplemented opcode: %ld (0x%08lx)\n",
               (long)lastop, (long)lastop);
	    syserr(buf);
	    }
	 }
	 continue;

C_rtn_term:
	 EntInterp;

	 switch (signal) {

	    case A_Resume:
	       goto efail_noev;

	    case A_Unmark_uw:		/* unwind for unmark */
	       goto Unmark_uw;

	    case A_Lsusp_uw:		/* unwind for lsusp */
	       goto Lsusp_uw;

	    case A_Eret_uw:		/* unwind for eret */
	       goto Eret_uw;

	    case A_Pret_uw:		/* unwind for pret */
	       goto Pret_uw;

	    case A_Pfail_uw:		/* unwind for pfail */
	       goto Pfail_uw;
	    }

	 rsp = (word *)rargp + 1;	/* set rsp to result */
	 continue;
	 }

interp_quit:
   --ilevel;
   if (ilevel != 0)
      syserr("interp: termination with inactive generators.");
   /*NOTREACHED*/
   return 0;	/* avoid gcc warning */
   }
