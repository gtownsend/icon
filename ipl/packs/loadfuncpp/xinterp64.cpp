/*
 * Tue Feb 12 18:19:56 2008
 * This file was produced by
 *   rtt: Icon Version 9.5.a-C, Autumn, 2007
 */
//and then modified by cs

 
extern "C" {	//cs 
 
#define COMPILER 0
#include RTT

//#line 22 "interp.r"

//word lastop;
extern word lastop;		//cs

//#line 28 "interp.r"

//struct ef_marker *efp;
extern struct ef_marker *efp;	//cs
//struct gf_marker *gfp;
extern struct gf_marker *gfp;	//cs
//inst ipc;
extern inst ipc;	//cs
//word *sp = NULL;
extern word *sp;	//cs

//int ilevel;
extern int ilevel;	//cs
//struct descrip value_tmp;
extern struct descrip value_tmp;	//cs
//struct descrip eret_tmp;
extern struct descrip eret_tmp;		//cs

//int coexp_act;
extern int coexp_act;	//cs

//#line 40 "interp.r"

//dptr xargp;
extern dptr xargp;		//cs
//word xnargs;
extern word xnargs;		//cs

//#line 155 "interp.r"

//int interp(fsig, cargp) 
//int fsig; 
//dptr cargp; 
static int icall(dptr procptr, dptr arglistptr, dptr result)		//cs 
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
//   extern int (*optab[])(); 
   extern int (*optab[])(dptr);		//cs
//   extern int (*keytab[])(); 
   extern int (*keytab[])(dptr);	//cs 
   struct b_proc *bproc;
	dptr lval;					//cs
	int fsig = 0;				//cs
	dptr cargp = (dptr)(sp+1);	//cs
	dptr return_cargp = cargp;	//cs
	word *saved_sp = sp;		//cs
	word *return_sp = sp + 2;	//cs

	cargp[0] = *procptr;		//cs
	cargp[1] = *arglistptr;		//cs
	sp += 4;					//cs
	
//#line 189 "interp.r"

   if (BlkLoc(k_current) == BlkLoc(k_main) && 
      ((char *)sp + PerilDelta) > (char *)stackend) 
      fatalerr(301, NULL);

//#line 195 "interp.r"

#if GPX	//cs
   if (!pollctr--) {
      pollctr = pollevent();
      if (pollctr == -1) fatalerr(141, NULL);
      }
#endif

//#line 201 "interp.r"

   ilevel++;

   rsp = sp;;

//#line 215 "interp.r"

   if (fsig == G_Csusp) {

//#line 218 "interp.r"

      oldsp = rsp;

//#line 223 "interp.r"

      newgfp = (struct gf_marker *)(rsp + 1);
      newgfp->gf_gentype = fsig;
      newgfp->gf_gfp = gfp;
      newgfp->gf_efp = efp;
      newgfp->gf_ipc = ipc;
      rsp += ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));

//#line 235 "interp.r"

      if (gfp != 0) {
         if (gfp->gf_gentype == G_Psusp) 
            firstwd = (word *)gfp + ((sizeof((*gfp)) + sizeof(word) - 1) / sizeof(word));
         else 
            firstwd = (word *)gfp + ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));
         }
      else 
         firstwd = (word *)efp + ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
      lastwd = (word *)cargp + 1;

//#line 249 "interp.r"

      for (wd = firstwd; wd <= lastwd; wd++) 
         *++rsp = *wd;
      gfp = newgfp;
      }

//#line 257 "interp.r"

	goto apply;	//cs

   for (; ; ) {

//#line 330 "interp.r"

      lastop = (word)(*ipc.op++);
      
	if( rsp < return_sp ) 	//cs
		syserror("loadfuncpp: call of Icon from C++ must return a value, yet failed instead");

//#line 348 "interp.r"

      switch ((int)lastop) {

//#line 359 "interp.r"

         case 51: 
            ipc.op[-1] = (90);
            PushValSP(rsp, D_Cset);
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;
            ipc.opnd[-1] = (opnd);
            PushValSP(rsp, opnd);
            break;

         case 90: 
            PushValSP(rsp, D_Cset);
            PushValSP(rsp, (*ipc.opnd++));
            break;

         case 60: 
            PushValSP(rsp, D_Integer);
            PushValSP(rsp, (*ipc.opnd++));
            break;

         case 75: 
            ipc.op[-1] = (91);
            PushValSP(rsp, D_Real);
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;
            PushValSP(rsp, opnd);
            ipc.opnd[-1] = (opnd);
            break;

         case 91: 
            PushValSP(rsp, D_Real);
            PushValSP(rsp, (*ipc.opnd++));
            break;

         case 77: 
            ipc.op[-1] = (92);
            PushValSP(rsp, (*ipc.opnd++));
            opnd = (word)strcons + (*ipc.opnd++);
            ipc.opnd[-1] = (opnd);
            PushValSP(rsp, opnd);
            break;

         case 92: 
            PushValSP(rsp, (*ipc.opnd++));
            PushValSP(rsp, (*ipc.opnd++));
            break;

//#line 407 "interp.r"

         case 81: 
            PushValSP(rsp, D_Var);
            PushValSP(rsp, &glbl_argp[(*ipc.opnd++) + 1]);
            break;

         case 84: 
            ipc.op[-1] = (93);
            PushValSP(rsp, D_Var);
            opnd = (*ipc.opnd++);
            PushValSP(rsp, &globals[opnd]);
            ipc.opnd[-1] = ((word)&globals[opnd]);
            break;

         case 93: 
            PushValSP(rsp, D_Var);
            PushValSP(rsp, (*ipc.opnd++));
            break;

         case 83: 
            PushValSP(rsp, D_Var);
            PushValSP(rsp, &pfp->pf_locals[(*ipc.opnd++)]);
            break;

         case 82: 
            ipc.op[-1] = (94);
            PushValSP(rsp, D_Var);
            opnd = (*ipc.opnd++);
            PushValSP(rsp, &statics[opnd]);
            ipc.opnd[-1] = ((word)&statics[opnd]);
            break;

         case 94: 
            PushValSP(rsp, D_Var);
            PushValSP(rsp, (*ipc.opnd++));
            break;

//#line 448 "interp.r"

         case 4: 
         case 19: 
         case 23: 
         case 34: 
         case 37: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 1;
            xargp = rargp;
            sp = rsp;;

//#line 453 "interp.r"

            ;
            Deref(rargp[1]);

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 455 "interp.r"

            ;

         case 43: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 1;
            xargp = rargp;
            sp = rsp;;

//#line 458 "interp.r"

            ;
            Deref(rargp[1]);

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 460 "interp.r"

            ;

         case 21: 
         case 22: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 1;
            xargp = rargp;
            sp = rsp;;

//#line 464 "interp.r"

            ;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 465 "interp.r"

            ;

         case 32: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 474 "interp.r"

         case 40: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 1;
            xargp = rargp;
            sp = rsp;;

//#line 475 "interp.r"

            ;
            Deref(rargp[1]);

//#line 105 "interp.r"

            signal = (*(optab[lastop]))(rargp);
            goto C_rtn_term;

//#line 477 "interp.r"

            ;

         case 2: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 481 "interp.r"

            ;

//#line 105 "interp.r"

            signal = (*(optab[lastop]))(rargp);
            goto C_rtn_term;

//#line 482 "interp.r"

            ;

//#line 486 "interp.r"

         case 3: 
         case 5: 
         case 6: 
         case 8: 
         case 9: 
         case 16: 
         case 17: 
         case 18: 
         case 31: 
         case 42: 
         case 30: 
         case 7: 
         case 10: 
         case 11: 
         case 12: 
         case 13: 
         case 14: 
         case 15: 
         case 20: 
         case 24: 
         case 25: 
         case 26: 
         case 27: 
         case 29: 
         case 28: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 511 "interp.r"

            ;
            Deref(rargp[1]);
            Deref(rargp[2]);

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 514 "interp.r"

            ;

         case 1: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 517 "interp.r"

            ;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 518 "interp.r"

            ;

         case 39: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 3;
            xargp = rargp;
            sp = rsp;;

//#line 522 "interp.r"

            ;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 523 "interp.r"

            ;

         case 38: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 3;
            xargp = rargp;
            sp = rsp;;

//#line 527 "interp.r"

            ;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 528 "interp.r"

            ;

//#line 531 "interp.r"

         case 33: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 532 "interp.r"

            ;

//#line 105 "interp.r"

            signal = (*(optab[lastop]))(rargp);
            goto C_rtn_term;

//#line 533 "interp.r"

            ;

         case 35: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 3;
            xargp = rargp;
            sp = rsp;;

//#line 537 "interp.r"

            ;

//#line 105 "interp.r"

            signal = (*(optab[lastop]))(rargp);
            goto C_rtn_term;

//#line 538 "interp.r"

            ;

//#line 542 "interp.r"

         case 36: 
            PushNullSP(rsp);

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 4;
            xargp = rargp;
            sp = rsp;;

//#line 544 "interp.r"

            ;

//#line 85 "interp.r"

            if ((*(optab[lastop]))(rargp) == A_Resume) {

//#line 89 "interp.r"

               goto efail_noev;
               }
            rsp = (word *)rargp + 1;

//#line 95 "interp.r"

            break;

//#line 545 "interp.r"

            ;

//#line 548 "interp.r"

         case 41: 

//#line 65 "interp.r"

            rargp = (dptr)(rsp - 1) - 3;
            xargp = rargp;
            sp = rsp;;

//#line 549 "interp.r"

            ;
            Deref(rargp[1]);
            Deref(rargp[2]);
            Deref(rargp[3]);

//#line 105 "interp.r"

            signal = (*(optab[lastop]))(rargp);
            goto C_rtn_term;

//#line 553 "interp.r"

            ;

         case 98: 

//#line 559 "interp.r"

#if GPX	//cs
            if (!pollctr--) {
               sp = rsp;;
               pollctr = pollevent();
               rsp = sp;;
               if (pollctr == -1) fatalerr(141, NULL);
               }
#endif

//#line 570 "interp.r"

            break;

//#line 573 "interp.r"

         case 108: 
            {

//#line 583 "interp.r"

            break;
            }

         case 64: 

//#line 590 "interp.r"

#if GPX	//cs
            if (!pollctr--) {
               sp = rsp;;
               pollctr = pollevent();
               rsp = sp;;
               if (pollctr == -1) fatalerr(141, NULL);
               }
#endif

//#line 606 "interp.r"

            break;

//#line 610 "interp.r"

         case 44: 
            PushDescSP(rsp, k_subject);
            PushValSP(rsp, D_Integer);
            PushValSP(rsp, k_pos);

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 614 "interp.r"

            ;

            signal = Obscan(2, rargp);

            goto C_rtn_term;

         case 55: 

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 1;
            xargp = rargp;
            sp = rsp;;

//#line 621 "interp.r"

            ;

            signal = Oescan(1, rargp);

            goto C_rtn_term;

//#line 629 "interp.r"

         case 89: {
		apply:		//cs
            union block *bp; 
            int i, j;

            value_tmp = *(dptr)(rsp - 1);
            Deref(value_tmp);
            switch (Type(value_tmp)) {
               case T_List: {
                  rsp -= 2;
                  bp = BlkLoc(value_tmp);
                  args = (int)bp->list.size;

//#line 647 "interp.r"

                  if (BlkLoc(k_current) == BlkLoc(k_main) && 
                     ((char *)sp + args * sizeof(struct descrip) > 
                     (char *)stackend)) 
                     fatalerr(301, NULL);

//#line 653 "interp.r"

                  for (bp = bp->list.listhead; 

//#line 657 "interp.r"

                  bp != NULL; 

                  bp = bp->lelem.listnext) {
                     for (i = 0; i < bp->lelem.nused; i++) {
                        j = bp->lelem.first + i;
                        if (j >= bp->lelem.nslots) 
                           j -= bp->lelem.nslots;
                        PushDescSP(rsp, bp->lelem.lslots[j]);
                        }
                     }
                  goto invokej;
                  }

               case T_Record: {
                  rsp -= 2;
                  bp = BlkLoc(value_tmp);
                  args = bp->record.recdesc->proc.nfields;
                  for (i = 0; i < args; i++) {
                     PushDescSP(rsp, bp->record.fields[i]);
                     }
                  goto invokej;
                  }

               default: {

                  xargp = (dptr)(rsp - 3);
                  err_msg(126, &value_tmp);
                  goto efail;
                  }
                  }
            }

         case 61: {
            args = (int)(*ipc.opnd++);
            invokej: 
            {
            int nargs; 
            dptr carg;

            sp = rsp;;
            type = invoke(args, &carg, &nargs);
            rsp = sp;;

            if (type == I_Fail) 
               goto efail_noev;
            if (type == I_Continue) 
               break;
            else {

               rargp = carg;

//#line 712 "interp.r"

#if GPX	//cs
               pollctr >>= 1;
               if (!pollctr) {
                  sp = rsp;;
                  pollctr = pollevent();
                  rsp = sp;;
                  if (pollctr == -1) fatalerr(141, NULL);
                  }
#endif

//#line 726 "interp.r"

               bproc = (struct b_proc *)BlkLoc(*rargp);

//#line 734 "interp.r"

               if (type == I_Vararg) {
//                  int (*bfunc)();
                  int (*bfunc)(int, dptr);	//cs
//                  bfunc = bproc->entryp.ccode;
                  bfunc = (int (*)(int,dptr))(bproc->entryp.ccode);

//#line 741 "interp.r"

                  signal = (*bfunc)(nargs, rargp);
                  }
               else 

//#line 746 "interp.r"

                  {
//                  int (*bfunc)();
                  int (*bfunc)(dptr);
//                  bfunc = bproc->entryp.ccode;
                  bfunc = (int (*)(dptr))(bproc->entryp.ccode);

//#line 753 "interp.r"

                  signal = (*bfunc)(rargp);
                  }

//#line 767 "interp.r"

               goto C_rtn_term;
               }
            }
            }

         case 62: 

            PushNullSP(rsp);
            opnd = (*ipc.opnd++);

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 0;
            xargp = rargp;
            sp = rsp;;

//#line 776 "interp.r"

            ;

            signal = (*(keytab[(int)opnd]))(rargp);
            goto C_rtn_term;

         case 65: 
            opnd = (*ipc.opnd++);

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - opnd;
            xargp = rargp;
            sp = rsp;;

//#line 793 "interp.r"

            ;

//#line 796 "interp.r"

            {
            int i;
            for (i = 1; i <= opnd; i++) 
               Deref(rargp[i]);
            }

            signal = Ollist((int)opnd, rargp);

            goto C_rtn_term;

//#line 808 "interp.r"

         case 67: 
            ipc.op[-1] = (96);
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;
            ipc.opnd[-1] = (opnd);
            newefp = (struct ef_marker *)(rsp + 1);
            newefp->ef_failure.opnd = (word *)opnd;
            goto mark;

         case 96: 
            newefp = (struct ef_marker *)(rsp + 1);
            newefp->ef_failure.opnd = (word *)(*ipc.opnd++);
            mark: 
            newefp->ef_gfp = gfp;
            newefp->ef_efp = efp;
            newefp->ef_ilevel = ilevel;
            rsp += ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
            efp = newefp;
            gfp = 0;
            break;

         case 85: 
            mark0: 
            newefp = (struct ef_marker *)(rsp + 1);
            newefp->ef_failure.opnd = 0;
            newefp->ef_gfp = gfp;
            newefp->ef_efp = efp;
            newefp->ef_ilevel = ilevel;
            rsp += ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
            efp = newefp;
            gfp = 0;
            break;

         case 78: 

//#line 849 "interp.r"

            gfp = efp->ef_gfp;
            rsp = (word *)efp - 1;

//#line 855 "interp.r"

            Unmark_uw: 
            if (efp->ef_ilevel < ilevel) {
               --ilevel;

               sp = rsp;;

//#line 866 "interp.r"

               return A_Unmark_uw;
               }

            efp = efp->ef_efp;
            break;

//#line 874 "interp.r"

         case 56: {

//#line 879 "interp.r"

            oldsp = rsp;
            newgfp = (struct gf_marker *)(rsp + 1);
            newgfp->gf_gentype = G_Esusp;
            newgfp->gf_gfp = gfp;
            newgfp->gf_efp = efp;
            newgfp->gf_ipc = ipc;
            gfp = newgfp;
            rsp += ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));

//#line 892 "interp.r"

            if (efp->ef_gfp != 0) {
               newgfp = (struct gf_marker *)(efp->ef_gfp);
               if (newgfp->gf_gentype == G_Psusp) 
                  firstwd = (word *)efp->ef_gfp + ((sizeof((*gfp)) + sizeof(word) - 1) / sizeof(word));
               else 
                  firstwd = (word *)efp->ef_gfp + 
                  ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));
               }
            else 
               firstwd = (word *)efp->ef_efp + ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
            lastwd = (word *)efp - 1;
            efp = efp->ef_efp;

//#line 909 "interp.r"

            for (wd = firstwd; wd <= lastwd; wd++) 
               *++rsp = *wd;
            PushValSP(rsp, oldsp[-1]);
            PushValSP(rsp, oldsp[0]);
            break;
            }

         case 66: {
            struct descrip sval; 

//#line 924 "interp.r"

//            dptr lval = (dptr)((word *)efp - 2);
			lval = (dptr)((word *)efp - 2);		//cs
			
//#line 929 "interp.r"

            if (--IntVal(*lval) > 0) {

//#line 934 "interp.r"

               sval = *(dptr)(rsp - 1);

//#line 941 "interp.r"

               if (efp->ef_gfp != 0) {
                  newgfp = (struct gf_marker *)(efp->ef_gfp);
                  if (newgfp->gf_gentype == G_Psusp) 
                     firstwd = (word *)efp->ef_gfp + ((sizeof((*gfp)) + sizeof(word) - 1) / sizeof(word));
                  else 
                     firstwd = (word *)efp->ef_gfp + 
                     ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));
                  }
               else 
                  firstwd = (word *)efp->ef_efp + ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
               lastwd = (word *)efp - 3;
               if (gfp == 0) 
                  gfp = efp->ef_gfp;
               efp = efp->ef_efp;

//#line 960 "interp.r"

               rsp -= 2;
               for (wd = firstwd; wd <= lastwd; wd++) 
                  *++rsp = *wd;
               PushDescSP(rsp, sval);
               }
            else {

//#line 973 "interp.r"

               *lval = *(dptr)(rsp - 1);

//#line 981 "interp.r"

               gfp = efp->ef_gfp;

//#line 987 "interp.r"

               Lsusp_uw: 
               if (efp->ef_ilevel < ilevel) {
                  --ilevel;
                  sp = rsp;;

//#line 997 "interp.r"

                  return A_Lsusp_uw;
                  }
               rsp = (word *)efp - 1;
               efp = efp->ef_efp;
               }
            break;
            }

         case 72: {

//#line 1015 "interp.r"

            struct descrip tmp; 
            dptr svalp; 
            struct b_proc *sproc;

//#line 1025 "interp.r"

            svalp = (dptr)(rsp - 1);
            if (Var(*svalp)) {
               sp = rsp;;
               retderef(svalp, (word *)glbl_argp, sp);
               rsp = sp;;
               }

//#line 1035 "interp.r"

            oldsp = rsp;
            newgfp = (struct gf_marker *)(rsp + 1);
            newgfp->gf_gentype = G_Psusp;
            newgfp->gf_gfp = gfp;
            newgfp->gf_efp = efp;
            newgfp->gf_ipc = ipc;
            newgfp->gf_argp = glbl_argp;
            newgfp->gf_pfp = pfp;
            gfp = newgfp;
            rsp += ((sizeof((*gfp)) + sizeof(word) - 1) / sizeof(word));

//#line 1051 "interp.r"

            if (pfp->pf_gfp != 0) {
               newgfp = (struct gf_marker *)(pfp->pf_gfp);
               if (newgfp->gf_gentype == G_Psusp) 
                  firstwd = (word *)pfp->pf_gfp + ((sizeof((*gfp)) + sizeof(word) - 1) / sizeof(word));
               else 
                  firstwd = (word *)pfp->pf_gfp + 
                  ((sizeof(struct gf_smallmarker) + sizeof(word) - 1) / sizeof(word));
               }
            else 
               firstwd = (word *)pfp->pf_efp + ((sizeof((*efp)) + sizeof(word) - 1) / sizeof(word));
            lastwd = (word *)glbl_argp - 1;
            efp = efp->ef_efp;

//#line 1068 "interp.r"

            for (wd = firstwd; wd <= lastwd; wd++) 
               *++rsp = *wd;
            PushValSP(rsp, oldsp[-1]);
            PushValSP(rsp, oldsp[0]);
            --k_level;
            if (k_trace) {
               k_trace--;
               sproc = (struct b_proc *)BlkLoc(*glbl_argp);
               strace(&(sproc->pname), svalp);
               }

//#line 1083 "interp.r"

            if (pfp->pf_scan != NULL) {

//#line 1089 "interp.r"

               tmp = k_subject;
               k_subject = *pfp->pf_scan;
               *pfp->pf_scan = tmp;

               tmp = *(pfp->pf_scan + 1);
               IntVal(*(pfp->pf_scan + 1)) = k_pos;
               k_pos = IntVal(tmp);
               }

//#line 1106 "interp.r"

            efp = pfp->pf_efp;
            ipc = pfp->pf_ipc;
            glbl_argp = pfp->pf_argp;
            pfp = pfp->pf_pfp;
            break;
            }

//#line 1115 "interp.r"

         case 54: {

//#line 1124 "interp.r"

            eret_tmp = *(dptr)&rsp[-1];
            gfp = efp->ef_gfp;
            Eret_uw: 

//#line 1131 "interp.r"

            if (efp->ef_ilevel < ilevel) {
               --ilevel;
               sp = rsp;;

//#line 1140 "interp.r"

               return A_Eret_uw;
               }
            rsp = (word *)efp - 1;
            efp = efp->ef_efp;
            PushDescSP(rsp, eret_tmp);
            break;
            }

//#line 1149 "interp.r"

         case 71: {

//#line 1163 "interp.r"

            struct b_proc *rproc;
            rproc = (struct b_proc *)BlkLoc(*glbl_argp);

//#line 1173 "interp.r"

            *glbl_argp = *(dptr)(rsp - 1);
            if (Var(*glbl_argp)) {
               sp = rsp;;
               retderef(glbl_argp, (word *)glbl_argp, sp);
               rsp = sp;;
               }

            --k_level;
            if (k_trace) {
               k_trace--;
               rtrace(&(rproc->pname), glbl_argp);
               }
            Pret_uw: 
            if (pfp->pf_ilevel < ilevel) {
               --ilevel;
               sp = rsp;;

//#line 1196 "interp.r"

               return A_Pret_uw;
               }

//#line 1203 "interp.r"

            rsp = (word *)glbl_argp + 1;
            efp = pfp->pf_efp;
            gfp = pfp->pf_gfp;
            ipc = pfp->pf_ipc;
            glbl_argp = pfp->pf_argp;
            pfp = pfp->pf_pfp;

//#line 1219 "interp.r"

//cs return to C++
			if( rsp == return_sp ) {
				//printf("Op_Pret caused a return to C++\n");fflush(stdout);
				--ilevel;
				*result = *return_cargp;
				sp = saved_sp;
				return 0;
			}
//cs end return to C++
            break;
            }

//#line 1224 "interp.r"

         case 53: 
            efail: 

//#line 1229 "interp.r"

            efail_noev: 

//#line 1233 "interp.r"

            if (gfp == 0) {

//#line 1251 "interp.r"

               ipc = efp->ef_failure;
               gfp = efp->ef_gfp;
               rsp = (word *)efp - 1;
               efp = efp->ef_efp;

               if (ipc.op == 0) 
                  goto efail;
               break;
               }
            else 
               {

//#line 1267 "interp.r"

               struct descrip tmp; 
               register struct gf_marker *resgfp = gfp;

               type = (int)resgfp->gf_gentype;

               if (type == G_Psusp) {
                  glbl_argp = resgfp->gf_argp;
                  if (k_trace) {
                     k_trace--;
                     sp = rsp;;
                     atrace(&(((struct b_proc *)BlkLoc(*glbl_argp))->pname));
                     rsp = sp;;
                     }
                  }
               ipc = resgfp->gf_ipc;
               efp = resgfp->gf_efp;
               gfp = resgfp->gf_gfp;
               rsp = (word *)resgfp - 1;
               if (type == G_Psusp) {
                  pfp = resgfp->gf_pfp;

//#line 1292 "interp.r"

                  if (pfp->pf_scan != NULL) {
                     tmp = k_subject;
                     k_subject = *pfp->pf_scan;
                     *pfp->pf_scan = tmp;

                     tmp = *(pfp->pf_scan + 1);
                     IntVal(*(pfp->pf_scan + 1)) = k_pos;
                     k_pos = IntVal(tmp);
                     }

//#line 1313 "interp.r"

                  ++k_level;
                  }

               switch (type) {

//#line 1336 "interp.r"

                  case G_Csusp: 
                     ;
                     --ilevel;
                     sp = rsp;;

//#line 1344 "interp.r"

                     return A_Resume;

                  case G_Esusp: 
                     ;
                     goto efail_noev;

                  case G_Psusp: 
                     ;
                     break;
                     }

               break;
               }

         case 68: {

//#line 1374 "interp.r"

            --k_level;
            if (k_trace) {
               k_trace--;
               failtrace(&(((struct b_proc *)BlkLoc(*glbl_argp))->pname));
               }
            Pfail_uw: 

            if (pfp->pf_ilevel < ilevel) {
               --ilevel;
               sp = rsp;;

//#line 1388 "interp.r"

               return A_Pfail_uw;
               }
            efp = pfp->pf_efp;
            gfp = pfp->pf_gfp;
            ipc = pfp->pf_ipc;
            glbl_argp = pfp->pf_argp;
            pfp = pfp->pf_pfp;

//#line 1406 "interp.r"

            goto efail_noev;
            }

//#line 1410 "interp.r"

         case 45: 
            PushNullSP(rsp);
            PushValSP(rsp, ((word *)efp)[-2]);
            PushValSP(rsp, ((word *)efp)[-1]);
            break;

         case 46: 
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;
            efp->ef_failure.opnd = (word *)opnd;
            break;

         case 52: 
            PushNullSP(rsp);
            rsp[1] = rsp[-3];
            rsp[2] = rsp[-2];
            rsp += 2;
            break;

         case 57: 
            PushValSP(rsp, D_Integer);
            PushValSP(rsp, (*ipc.opnd++));

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 2;
            xargp = rargp;
            sp = rsp;;

//#line 1432 "interp.r"

            ;

            signal = Ofield(2, rargp);

            goto C_rtn_term;

         case 58: 
            ipc.op[-1] = (95);
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;
            ipc.opnd[-1] = (opnd);
            ipc.opnd = (word *)opnd;
            break;

         case 95: 
            opnd = (*ipc.opnd++);
            ipc.opnd = (word *)opnd;
            break;

         case 59: 
            *--ipc.op = 58;
            opnd = sizeof((*ipc.op)) + sizeof((*rsp));
            opnd += (word)ipc.opnd;
            ipc.opnd = (word *)opnd;
            break;

         case 63: 

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 0;
            xargp = rargp;
            sp = rsp;;

//#line 1459 "interp.r"

            ;

            if (Olimit(0, rargp) == A_Resume) {

//#line 1468 "interp.r"

               goto efail_noev;
               }
            else {

//#line 1476 "interp.r"

               rsp = (word *)rargp + 1;
               }
            goto mark0;

//#line 1486 "interp.r"

         case 69: 
            PushNullSP(rsp);
            break;

         case 70: 
            rsp -= 2;
            break;

         case 73: 
            PushValSP(rsp, D_Integer);
            PushValSP(rsp, 1);
            break;

         case 74: 
            PushValSP(rsp, D_Integer);
            PushValSP(rsp, -1);
            break;

         case 76: 
            rsp += 2;
            rsp[-1] = rsp[-3];
            rsp[0] = rsp[-2];
            break;

//#line 1512 "interp.r"

         case 50: 

//#line 1515 "interp.r"

            PushNullSP(rsp);

//#line 79 "interp.r"

            rargp = (dptr)(rsp - 1) - 0;
            xargp = rargp;
            sp = rsp;;

//#line 1516 "interp.r"

            ;
            opnd = (*ipc.opnd++);
            opnd += (word)ipc.opnd;

            signal = Ocreate((word *)opnd, rargp);

            goto C_rtn_term;

//#line 1528 "interp.r"

         case 47: {

//#line 1534 "interp.r"

            struct b_coexpr *ncp; 
            dptr dp;

            sp = rsp;;
            dp = (dptr)(sp - 1);
            xargp = dp - 2;

            Deref(*dp);
            if (dp->dword != D_Coexpr) {
               err_msg(118, dp);
               goto efail;
               }

            ncp = (struct b_coexpr *)BlkLoc(*dp);

            signal = activate((dptr)(sp - 3), ncp, (dptr)(sp - 3));
            rsp = sp;;
            if (signal == A_Resume) 
               goto efail_noev;
            else 
               rsp -= 2;

            break;
            }

         case 49: {

//#line 1564 "interp.r"

            struct b_coexpr *ncp;

            sp = rsp;;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            ++BlkLoc(k_current)->coexpr.size;
            co_chng(ncp, (dptr)&sp[-1], NULL, A_Coret, 1);
            rsp = sp;;

            break;
            }

//#line 1577 "interp.r"

         case 48: {

//#line 1582 "interp.r"

            struct b_coexpr *ncp;

            sp = rsp;;
            ncp = popact((struct b_coexpr *)BlkLoc(k_current));

            co_chng(ncp, NULL, NULL, A_Cofail, 1);
            rsp = sp;;

            break;
            }

         case 86: 

//#line 1596 "interp.r"

            goto interp_quit;

//#line 1599 "interp.r"

         default: {
            char buf[50];

            sprintf(buf, "unimplemented opcode: %ld (0x%08x)\n", 
            (long)lastop, lastop);
            syserr(buf);
            }
            }
      continue;

      C_rtn_term: 
      rsp = sp;;

      switch (signal) {

         case A_Resume: 

//#line 1622 "interp.r"

            goto efail_noev;

         case A_Unmark_uw: 

//#line 1631 "interp.r"

            goto Unmark_uw;

         case A_Lsusp_uw: 

//#line 1640 "interp.r"

            goto Lsusp_uw;

         case A_Eret_uw: 

//#line 1649 "interp.r"

            goto Eret_uw;

         case A_Pret_uw: 

//#line 1658 "interp.r"

            goto Pret_uw;

         case A_Pfail_uw: 

//#line 1667 "interp.r"

            goto Pfail_uw;
            }

      rsp = (word *)rargp + 1;

//#line 1682 "interp.r"

      continue;
      }

   interp_quit: 
   --ilevel;
   if (ilevel != 0) 
      syserror("interp: termination with inactive generators.");

   return 0;
   }

}	//cs --- extern "C"
