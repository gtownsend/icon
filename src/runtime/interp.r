#if !COMPILER
/*
 * File: interp.r
 *  The interpreter proper.
 */

#include "../h/opdefs.h"

extern fptr fncentry[];


/*
 * Prototypes for static functions.
 */
#ifdef EventMon
static struct ef_marker *vanq_bound (struct ef_marker *efp_v,
                                      struct gf_marker *gfp_v);
static void           vanq_proc (struct ef_marker *efp_v,
                                     struct gf_marker *gfp_v);
#endif					/* EventMon */

#ifndef MultiThread
word lastop;			/* Last operator evaluated */
#endif					/* MultiThread */

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

#ifndef MultiThread
dptr xargp;
word xnargs;
#endif					/* MultiThread */

/*
 * Macros for use inside the main loop of the interpreter.
 */

#ifdef EventMon
#define E_Misc    -1
#define E_Operator 0
#define E_Function 1
#endif					/* EventMon */

/*
 * Setup_Op sets things up for a call to the C function for an operator.
 *  InterpEVValD expands to nothing if EventMon is not defined.
 */
#begdef Setup_Op(nargs)
#ifdef EventMon
   lastev = E_Operator;
   value_tmp.dword = D_Proc;
   value_tmp.vword.bptr = (union block *)&op_tbl[lastop - 1];
   InterpEVValD(&value_tmp, E_Ocall);
#endif					/* EventMon */
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
#ifdef EventMon
   lastev = E_Misc;
#endif					/* EventMon */
   rargp = (dptr)(rsp - 1) - nargs;
   xargp = rargp;
   ExInterp;
#enddef					/* Setup_Arg */

#begdef Call_Cond
   if ((*(optab[lastop]))(rargp) == A_Resume) {
#ifdef EventMon
     InterpEVVal((word)-1, E_Ofail);
#endif					/* EventMon */
     goto efail_noev;
   }
   rsp = (word *) rargp + 1;
#ifdef EventMon
   goto return_term;
#else					/* EventMon */
   break;
#endif					/* EventMon */
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
#ifdef EventMon
   int lastev = E_Misc;
#endif					/* EventMon */

#ifdef TallyOpt
   extern word tallybin[];
#endif					/* TallyOpt */

#ifdef EventMon
   EVVal(fsig, E_Intcall);
   EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */

#ifndef MultiThread
   /*
    * Make a stab at catching interpreter stack overflow.  This does
    * nothing for invocation in a co-expression other than &main.
    */
   if (BlkLoc(k_current) == BlkLoc(k_main) &&
      ((char *)sp + PerilDelta) > (char *)stackend)
         fatalerr(301, NULL);
#endif					/* MultiThread */

#ifdef Polling
   if (!pollctr--) {
      pollctr = pollevent();
      if (pollctr == -1) fatalerr(141, NULL);
      }
#endif					/* Polling */

   ilevel++;

   EntInterp;

#ifdef EventMon
   switch (fsig) {
   case G_Csusp:
   case G_Fsusp:
   case G_Osusp:
      value_tmp = *(dptr)(rsp - 1);	/* argument */
      Deref(value_tmp);
      InterpEVValD(&value_tmp,
		   (fsig == G_Fsusp)?E_Fsusp:(fsig == G_Osusp?E_Osusp:E_Bsusp));
#else					/* EventMon */
   if (fsig == G_Csusp) {
#endif					/* EventMon */

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

#ifdef EventMon

   /*
    * Location change events are generated by checking to see if the opcode
    *  has changed indices in the "line number" (now line + column) table;
    *  "straight line" forward code does not require a binary search to find
    *  the new location; instead, a pointer is simply incremented.
    *  Further optimization here is planned.
    */
   if (!is:null(curpstate->eventmask) && (
       Testb((word)E_Loc, curpstate->eventmask) ||
       Testb((word)E_Line, curpstate->eventmask)
       )) {

      if (InRange(code, ipc.opnd, ecode)) {
	uword ipc_offset = DiffPtrs((char *)ipc.opnd, (char *)code);
	uword size;
	word temp_no;
	if (!current_line_ptr ||
	    current_line_ptr->ipc > ipc_offset ||
	    current_line_ptr[1].ipc <= ipc_offset) {
#ifdef LineCodes
#ifdef Polling
            if (!pollctr--) {
	       ExInterp;
               pollctr = pollevent();
	       EntInterp;
	       if (pollctr == -1) fatalerr(141, NULL);
	       }
#endif					/* Polling */
#endif					/* LineCodes */


	    if(current_line_ptr &&
	       current_line_ptr + 2 < elines &&
	       current_line_ptr[1].ipc < ipc_offset &&
	       ipc_offset < current_line_ptr[2].ipc) {
	       current_line_ptr ++;
	       }
	    else {
	       current_line_ptr = ilines;
	       size = DiffPtrs((char *)elines, (char *)ilines) /
		  sizeof(struct ipc_line *);
	       while (size > 1) {
		  if (ipc_offset >= current_line_ptr[size>>1].ipc) {
		     current_line_ptr = &current_line_ptr[size>>1];
		     size -= (size >> 1);
		     }
		  else {
		     size >>= 1;
		     }
		  }
	       }
	    linenum = current_line_ptr->line;
            temp_no = linenum & 65535;
            if ((lastline & 65535) != temp_no) {
               if (Testb((word)E_Line, curpstate->eventmask))
                     if (temp_no)
                        InterpEVVal(temp_no, E_Line);
	       }
	    if (lastline != linenum) {
	       lastline = linenum;
	       if (Testb((word)E_Loc, curpstate->eventmask) &&
		   current_line_ptr->line >> 16)
		  InterpEVVal(current_line_ptr->line, E_Loc);
	       }
	    }
	}
      }
#endif					/* EventMon */

      lastop = GetOp;		/* Instruction fetch */

#ifdef EventMon
      /*
       * If we've asked for ALL opcode events, or specifically for this one
       * generate an MT-style event.
       */
      if ((!is:null(curpstate->eventmask) &&
	   Testb((word)E_Opcode, curpstate->eventmask)) &&
	  (is:null(curpstate->opcodemask) ||
	   Testb((word)lastop, curpstate->opcodemask))) {
	 ExInterp;
	 MakeInt(lastop, &(curpstate->parent->eventval));
	 actparent(E_Opcode);
	 EntInterp
	 }
#endif					/* EventMon */

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

         case Op_Noop:		/* no-op */

#ifdef LineCodes
#ifdef Polling
            if (!pollctr--) {
	       ExInterp;
               pollctr = pollevent();
	       EntInterp;
	       if (pollctr == -1) fatalerr(141, NULL);
	       }
#endif					/* Polling */


#endif					/* LineCodes */

            break;


         case Op_Colm:		/* source column number */
            {
#ifdef EventMon
            word loc;
            column = GetWord;
            loc = column;
            loc <<= (WordBits >> 1);	/* column in high-order part */
            loc += linenum;
            InterpEVVal(loc, E_Loc);
#endif					/* EventMon */
            break;
            }

         case Op_Line:		/* source line number */

#ifdef LineCodes
#ifdef Polling
            if (!pollctr--) {
	       ExInterp;
               pollctr = pollevent();
	       EntInterp;
	       if (pollctr == -1) fatalerr(141, NULL);
	       }
#endif					/* Polling */


#endif					/* LineCodes */

#ifdef EventMon
            linenum = GetWord;
            lastline = linenum;
#endif					/* EventMon */

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

#ifndef MultiThread
                 /*
                  * Make a stab at catching interpreter stack overflow.
                  * This does nothing for invocation in a co-expression other
                  * than &main.
                  */
                 if (BlkLoc(k_current) == BlkLoc(k_main) &&
                    ((char *)sp + args * sizeof(struct descrip) >
                       (char *)stackend))
                          fatalerr(301, NULL);
#endif					/* MultiThread */

                  for (bp = bp->list.listhead;
#ifdef ListFix
		       BlkType(bp) == T_Lelem;
#else					/* ListFix */
		       bp != NULL;
#endif					/* ListFix */
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

#ifdef Polling
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
#endif					/* Polling */

#ifdef EventMon
	       lastev = E_Function;
	       InterpEVValD(rargp, E_Fcall);
#endif					/* EventMon */

	       bproc = (struct b_proc *)BlkLoc(*rargp);

#ifdef FncTrace
               typedef int (*bfunc2)(dptr, struct descrip *);
#endif					/* FncTrace */


	       /* ExInterp not needed since no change since last EntInterp */
	       if (type == I_Vararg) {
	          int (*bfunc)();
                  bfunc = bproc->entryp.ccode;

#ifdef FncTrace
                  signal = (*bfunc)(nargs, rargp, &(procs->pname));
#else					/* FncTrace */
		  signal = (*bfunc)(nargs,rargp);
#endif					/* FncTrace */

                  }
	       else
                  {
                  int (*bfunc)();
                  bfunc = bproc->entryp.ccode;

#ifdef FncTrace
                  signal = (*(bfunc2)bfunc)(rargp, &(bproc->pname));
#else					/* FncTrace */
		  signal = (*bfunc)(rargp);
#endif					/* FncTrace */
                  }

#ifdef FncTrace
               if (k_ftrace) {
                  k_ftrace--;
                  if (signal == A_Failure)
                     failtrace(&(bproc->pname));
                  else
                     rtrace(&(bproc->pname),rargp);
                  }
#endif					/* FncTrace */

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

#ifdef EventMon
            lastev = E_Operator;
            value_tmp.dword = D_Proc;
            value_tmp.vword.bptr = (union block *)&mt_llist;
            InterpEVValD(&value_tmp, E_Ocall);
            rargp = (dptr)(rsp - 1) - opnd;
            xargp = rargp;
            ExInterp;
#else					/* EventMon */
	    Setup_Arg(opnd);
#endif					/* EventMon */

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

#ifdef EventMon
	    ExInterp;
            vanq_bound(efp, gfp);
	    EntInterp;
#endif					/* EventMon */

	    gfp = efp->ef_gfp;
	    rsp = (word *)efp - 1;

	    /*
	     * Remove any suspended C generators.
	     */
Unmark_uw:
	    if (efp->ef_ilevel < ilevel) {
	       --ilevel;

	       ExInterp;

#ifdef EventMon
	       EVVal(A_Unmark_uw, E_Intret);
               EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */

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

#ifdef EventMon
	       ExInterp;
               vanq_bound(efp, gfp);
	       EntInterp;
#endif					/* EventMon */

	       gfp = efp->ef_gfp;

	       /*
		* Since an expression frame is being removed, inactive
		*  C generators contained therein are deactivated.
		*/
Lsusp_uw:
	       if (efp->ef_ilevel < ilevel) {
		  --ilevel;
		  ExInterp;

#ifdef EventMon
                  EVVal(A_Lsusp_uw, E_Intret);
                  EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */

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

#ifdef EventMon
            value_tmp = *(dptr)(rsp - 1);	/* argument */
            Deref(value_tmp);
            InterpEVValD(&value_tmp, E_Psusp);
#endif					/* EventMon */

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

#ifdef EventMon
	       InterpEVValD(&k_subject, E_Ssusp);
#endif					/* EventMon */

	       tmp = k_subject;
	       k_subject = *pfp->pf_scan;
	       *pfp->pf_scan = tmp;

	       tmp = *(pfp->pf_scan + 1);
	       IntVal(*(pfp->pf_scan + 1)) = k_pos;
	       k_pos = IntVal(tmp);
	       }

#ifdef MultiThread
	    /*
	     * If the program state changed for this procedure call,
	     * change back.
	     */
	    ENTERPSTATE(pfp->pf_prog);
#endif					/* MultiThread */

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

#ifdef EventMon
               EVVal(A_Eret_uw, E_Intret);
               EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */

	       return A_Eret_uw;
	       }
	    rsp = (word *)efp - 1;
	    efp = efp->ef_efp;
	    PushDesc(eret_tmp);
	    break;
	    }


	 case Op_Pret: {	/* return from procedure */
#ifdef EventMon
	   struct descrip oldargp;
	   static struct descrip unwinder;
#endif					/* EventMon */

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
#ifdef EventMon
            oldargp = *glbl_argp;
	    ExInterp;
            vanq_proc(efp, gfp);
	    EntInterp;
	    /* used to InterpEVValD(argp,E_Pret); here */
#endif					/* EventMon */

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

#ifdef EventMon
               EVVal(A_Pret_uw, E_Intret);
               EVVal(DiffPtrs(sp, stack), E_Stack);
	       unwinder = oldargp;
#endif					/* EventMon */

	       return A_Pret_uw;
	       }

#ifdef EventMon
	   if (!is:proc(oldargp) && is:proc(unwinder))
	      oldargp = unwinder;
#endif					/* EventMon */
	    rsp = (word *)glbl_argp + 1;
	    efp = pfp->pf_efp;
	    gfp = pfp->pf_gfp;
	    ipc = pfp->pf_ipc;
	    glbl_argp = pfp->pf_argp;
	    pfp = pfp->pf_pfp;

#ifdef MultiThread
	    if (pfp)
	       ENTERPSTATE(pfp->pf_prog);
#ifdef EventMon
            value_tmp = *(dptr)(rsp - 1);	/* argument */
            Deref(value_tmp);
            InterpEVValD(&value_tmp, E_Pret);
#endif					/* EventMon */
#endif					/* MultiThread */
	    break;
	    }

				/* ---Failures--- */

	 case Op_Efail:
efail:
#ifdef EventMon
            InterpEVVal((word)-1, E_Efail);
#endif					/* EventMon */
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

#ifdef MultiThread
	      if (efp == 0) {
		 break;
	         }
#endif					/* MultiThread */

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

#ifdef EventMon
		     InterpEVValD(&k_subject, E_Sresum);
#endif					/* EventMon */
		     }

#ifdef MultiThread
		  /*
		   * Enter the program state of the resumed frame
		   */
		  ENTERPSTATE(pfp->pf_prog);
#endif					/* MultiThread */

		  ++k_level;		/* adjust procedure level */
		  }

	       switch (type) {

#ifdef EventMon
		  case G_Fsusp:
                     InterpEVVal((word)0, E_Fresum);
		     --ilevel;
		     ExInterp;
                     EVVal(A_Resume, E_Intret);
                     EVVal(DiffPtrs(sp, stack), E_Stack);
		     return A_Resume;

		  case G_Osusp:
                     InterpEVVal((word)0, E_Oresum);
		     --ilevel;
		     ExInterp;
                     EVVal(A_Resume, E_Intret);
                     EVVal(DiffPtrs(sp, stack), E_Stack);
		     return A_Resume;
#endif					/* EventMon */

		  case G_Csusp:
                     InterpEVVal((word)0, E_Eresum);
		     --ilevel;
		     ExInterp;
#ifdef EventMon
                     EVVal(A_Resume, E_Intret);
                     EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */
		     return A_Resume;

		  case G_Esusp:
                     InterpEVVal((word)0, E_Eresum);
		     goto efail_noev;

		  case G_Psusp:		/* resuming a procedure */
                     InterpEVValD(glbl_argp, E_Presum);
		     break;
		  }

	       break;
	       }

	 case Op_Pfail: {	/* fail from procedure */

#ifdef EventMon
	    ExInterp;
            vanq_proc(efp, gfp);
            EVValD(glbl_argp, E_Pfail);
	    EntInterp;
#endif					/* EventMon */

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
#ifdef EventMon
               EVVal(A_Pfail_uw, E_Intret);
               EVVal(DiffPtrs(sp, stack), E_Stack);
#endif					/* EventMon */
	       return A_Pfail_uw;
	       }
	    efp = pfp->pf_efp;
	    gfp = pfp->pf_gfp;
	    ipc = pfp->pf_ipc;
	    glbl_argp = pfp->pf_argp;
	    pfp = pfp->pf_pfp;

#ifdef MultiThread
	    /*
	     * Enter the program state of the procedure being reentered.
	     * A NULL pfp indicates the program is complete.
	     */
	    if (pfp) {
	       ENTERPSTATE(pfp->pf_prog);
	       }
#endif					/* MultiThread */

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

#ifdef TallyOpt
	 case Op_Tally:		/* tally */
	    tallybin[GetWord]++;
	    break;
#endif					/* TallyOpt */

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

#ifdef Coexpr
	    PushNull;
	    Setup_Arg(0);
	    opnd = GetWord;
	    opnd += (word)ipc.opnd;

	    signal = Ocreate((word *)opnd, rargp);

	    goto C_rtn_term;
#else					/* Coexpr */
	    err_msg(401, NULL);
	    goto efail;
#endif					/* Coexpr */

	 case Op_Coact: {	/* @e */

#ifndef Coexpr
            err_msg(401, NULL);
            goto efail;
#else					/* Coexpr */
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
#endif					/* Coexpr */
            break;
	    }

	 case Op_Coret: {	/* return from co-expression */

#ifndef Coexpr
            syserr("co-expression return, but co-expressions not implemented");
#else					/* Coexpr */
            struct b_coexpr *ncp;

            ExInterp;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            ++BlkLoc(k_current)->coexpr.size;
            co_chng(ncp, (dptr)&sp[-1], NULL, A_Coret, 1);
            EntInterp;
#endif					/* Coexpr */
            break;

	    }

	 case Op_Cofail: {	/* fail from co-expression */

#ifndef Coexpr
            syserr("co-expression failure, but co-expressions not implemented");
#else					/* Coexpr */
            struct b_coexpr *ncp;

            ExInterp;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            co_chng(ncp, NULL, NULL, A_Cofail, 1);
            EntInterp;
#endif					/* Coexpr */
            break;

	    }
         case Op_Quit:		/* quit */


	    goto interp_quit;


	 default: {
	    char buf[50];

	    sprintf(buf, "unimplemented opcode: %ld (0x%08x)\n",
               (long)lastop, lastop);
	    syserr(buf);
	    }
	 }
	 continue;

C_rtn_term:
	 EntInterp;

	 switch (signal) {

	    case A_Resume:
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)-1,
			      ((lastev == E_Function)? E_Ffail : E_Ofail));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto efail_noev;

	    case A_Unmark_uw:		/* unwind for unmark */
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)0, ((lastev==E_Function) ? E_Frem:E_Orem));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto Unmark_uw;

	    case A_Lsusp_uw:		/* unwind for lsusp */
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)0, ((lastev==E_Function) ? E_Frem:E_Orem));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto Lsusp_uw;

	    case A_Eret_uw:		/* unwind for eret */
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)0, ((lastev==E_Function) ? E_Frem:E_Orem));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto Eret_uw;

	    case A_Pret_uw:		/* unwind for pret */
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)0, ((lastev==E_Function) ? E_Frem:E_Orem));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto Pret_uw;

	    case A_Pfail_uw:		/* unwind for pfail */
#ifdef EventMon
	       if ((lastev == E_Function) || (lastev == E_Operator)) {
		  InterpEVVal((word)0, ((lastev==E_Function) ? E_Frem:E_Orem));
		  lastev = E_Misc;
		  }
#endif					/* EventMon */
	       goto Pfail_uw;
	    }

	 rsp = (word *)rargp + 1;	/* set rsp to result */

#ifdef EventMon
return_term:
         value_tmp = *(dptr)(rsp - 1);	/* argument */
         Deref(value_tmp);
         if ((lastev == E_Function) || (lastev == E_Operator)) {
	    InterpEVValD(&value_tmp, ((lastev == E_Function) ? E_Fret:E_Oret));
	    lastev = E_Misc;
	    }
#endif					/* EventMon */

	 continue;
	 }

interp_quit:
   --ilevel;
   if (ilevel != 0)
      syserr("interp: termination with inactive generators.");
   /*NOTREACHED*/
   return 0;	/* avoid gcc warning */
   }

#ifdef EventMon
/*
 * vanq_proc - monitor the removal of suspended operations from within
 *   a procedure.
 */
static void vanq_proc(efp_v, gfp_v)
struct ef_marker *efp_v;
struct gf_marker *gfp_v;
   {

   if (is:null(curpstate->eventmask))
      return;

   /*
    * Go through all the bounded expression of the procedure.
    */
   while ((efp_v = vanq_bound(efp_v, gfp_v)) != NULL) {
      gfp_v = efp_v->ef_gfp;
      efp_v = efp_v->ef_efp;
      }
   }

/*
 * vanq_bound - monitor the removal of suspended operations from
 *   the current bounded expression and return the expression frame
 *   pointer for the bounded expression.
 */
static struct ef_marker *vanq_bound(efp_v, gfp_v)
struct ef_marker *efp_v;
struct gf_marker *gfp_v;
   {

   if (is:null(curpstate->eventmask))
      return efp_v;

   while (gfp_v != 0) {		/* note removal of suspended operations */
      switch ((int)gfp_v->gf_gentype) {
         case G_Psusp:
            EVValD(gfp_v->gf_argp, E_Prem);
            break;
	 /* G_Fsusp and G_Osusp handled in-line during unwinding */
         case G_Esusp:
            EVVal((word)0, E_Erem);
            break;
         }

      if (((int)gfp_v->gf_gentype) == G_Psusp) {
         vanq_proc(gfp_v->gf_efp, gfp_v->gf_gfp);
         efp_v = gfp_v->gf_pfp->pf_efp;           /* efp before the call */
         gfp_v = gfp_v->gf_pfp->pf_gfp;           /* gfp before the call */
         }
      else {
         efp_v = gfp_v->gf_efp;
         gfp_v = gfp_v->gf_gfp;
         }
      }

   return efp_v;
   }
#endif					/* EventMon */

#ifdef MultiThread
/*
 * activate some other co-expression from an arbitrary point in
 * the interpreter.
 */
int mt_activate(tvalp,rslt,ncp)
dptr tvalp, rslt;
register struct b_coexpr *ncp;
{
   register struct b_coexpr *ccp = (struct b_coexpr *)BlkLoc(k_current);
   int first, rv;

   dptr savedtvalloc = NULL;
   /*
    * Set activator in new co-expression.
    */
   if (ncp->es_actstk == NULL) {
      Protect(ncp->es_actstk = alcactiv(), { err_msg(0, NULL); exit(1); });
      /*
       * If no one ever explicitly activates this co-expression, fail to
       * the implicit activator.
       */
      ncp->es_actstk->arec[0].activator = ccp;
      first = 0;
      }
   else
      first = 1;

   if(ccp->tvalloc) {
     if (InRange(blkbase,ccp->tvalloc,blkfree)) {
       fprintf(stderr,
	       "Multiprogram garbage collection disaster in mt_activate()!\n");
       fflush(stderr);
       exit(1);
     }
     savedtvalloc = ccp->tvalloc;
   }

   rv = co_chng(ncp, tvalp, rslt, A_MTEvent, first);

   if ((savedtvalloc != NULL) && (savedtvalloc != ccp->tvalloc)) {
      fprintf(stderr,"averted co-expression disaster in activate\n");
      ccp->tvalloc = savedtvalloc;
      }

   return rv;
}


/*
 * activate the "&parent" co-expression from anywhere, if there is one
 */
void actparent(event)
int event;
   {
   struct progstate *parent = curpstate->parent;

   StrLen(parent->eventcode) = 1;
   StrLoc(parent->eventcode) = (char *)&allchars[event & 0xFF];
   mt_activate(&(parent->eventcode), NULL,
	       (struct b_coexpr *)curpstate->parent->Mainhead);
   }
#endif					/* MultiThread */
#endif					/* !COMPILER */
