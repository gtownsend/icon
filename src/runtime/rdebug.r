/*
 * rdebug.r - tracebk, get_name, xdisp, ctrace, rtrace, failtrace, strace,
 *   atrace, cotrace
 */

/*
 * Prototypes.
 */
static int     glbcmp    (char *pi, char *pj);
static int     keyref    (union block *bp, dptr dp);
static void showline  (char *f, int l);
static void showlevel (register int n);
static void ttrace	(void);
static void xtrace
   (struct b_proc *bp, word nargs, dptr arg, int pline, char *pfile);

/*
 * tracebk - print a trace of procedure calls.
 */

void tracebk(struct pf_marker *lcl_pfp, dptr argp)
   {
   struct b_proc *cproc;

   struct pf_marker *origpfp = pfp;
   dptr arg;
   inst cipc;

   /*
    * Chain back through the procedure frame markers, looking for the
    *  first one, while building a foward chain of pointers through
    *  the expression frame pointers.
    */

   for (pfp->pf_efp = NULL; pfp->pf_pfp != NULL; pfp = pfp->pf_pfp) {
      (pfp->pf_pfp)->pf_efp = (struct ef_marker *)pfp;
      }

   /* Now start from the base procedure frame marker, producing a listing
    *  of the procedure calls up through the last one.
    */

   while (pfp) {
      arg = &((dptr)pfp)[-(pfp->pf_nargs) - 1];
      cproc = (struct b_proc *)BlkLoc(arg[0]);
      /*
       * The ipc in the procedure frame points after the "invoke n".
       */
      cipc = pfp->pf_ipc;
      --cipc.opnd;
      --cipc.op;

      xtrace(cproc, pfp->pf_nargs, &arg[0], findline(cipc.opnd),
         findfile(cipc.opnd));
      /*
       * On the last call, show both the call and the offending expression.
       */
      if (pfp == origpfp) {
         ttrace();
         break;
         }

      pfp = (struct pf_marker *)(pfp->pf_efp);
      }
   }

/*
 * xtrace - procedure *bp is being called with nargs arguments, the first
 *  of which is at arg; produce a trace message.
 */
static void xtrace(struct b_proc *bp,
   word nargs, dptr arg, int pline, char *pfile)
   {

   if (bp == NULL)
      fprintf(stderr, "????");
   else {
       if (arg[0].dword == D_Proc)
          putstr(stderr, &(bp->pname));
       else
          outimage(stderr, arg, 0);
       arg++;

       putc('(', stderr);
       while (nargs--) {
          outimage(stderr, arg++, 0);
          if (nargs)
             putc(',', stderr);
          }
       putc(')', stderr);
       }

   if (pline != 0)
      fprintf(stderr, " from line %d in %s", pline, pfile);
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * get_name -- function to get print name of variable.
 */
int get_name(dptr dp1, dptr dp0)
   {
   dptr dp, varptr;
   tended union block *blkptr;
   dptr arg1;                           /* 1st parameter */
   dptr loc1;                           /* 1st local */
   struct b_proc *proc;                 /* address of procedure block */
   char sbuf[100];			/* buffer; might be too small */
   char *s, *s2;
   word i, j, k;
   int t;

   arg1 = &glbl_argp[1];
   loc1 = pfp->pf_locals;
   proc = &BlkLoc(*glbl_argp)->proc;

   type_case *dp1 of {
      tvsubs: {
         blkptr = BlkLoc(*dp1);
         get_name(&(blkptr->tvsubs.ssvar),dp0);
         sprintf(sbuf,"[%ld:%ld]",(long)blkptr->tvsubs.sspos,
            (long)blkptr->tvsubs.sspos+blkptr->tvsubs.sslen);
         k = StrLen(*dp0);
         j = strlen(sbuf);

	 /*
	  * allocate space for both the name and the subscript image,
	  *  and then copy both parts into the allocated space
	  */
	 Protect(s = alcstr(NULL, k + j), return Error);
	 s2 = StrLoc(*dp0);
	 StrLoc(*dp0) = s;
         StrLen(*dp0) = j + k;
	 for (i = 0; i < k; i++)
	    *s++ = *s2++;
	 s2 = sbuf;
	 for (i = 0; i < j; i++)
	    *s++ = *s2++;
         }

      tvtbl: {
         t = keyref(BlkLoc(*dp1) ,dp0);
         if (t == Error)
            return Error;
          }

      kywdint:
         if (VarLoc(*dp1) == &kywd_ran) {
            StrLen(*dp0) = 7;
            StrLoc(*dp0) = "&random";
            }
         else if (VarLoc(*dp1) == &kywd_trc) {
            StrLen(*dp0) = 6;
            StrLoc(*dp0) = "&trace";
            }
         else if (VarLoc(*dp1) == &kywd_dmp) {
            StrLen(*dp0) = 5;
            StrLoc(*dp0) = "&dump";
            }
         else if (VarLoc(*dp1) == &kywd_err) {
            StrLen(*dp0) = 6;
            StrLoc(*dp0) = "&error";
            }
         else
            syserr("name: unknown integer keyword variable");

      kywdevent:
            syserr("name: unknown event keyword variable");

      kywdwin: {
         StrLen(*dp0) = 7;
         StrLoc(*dp0) = "&window";
         }

      kywdstr: {
         StrLen(*dp0) = 9;
         StrLoc(*dp0) = "&progname";
         }

      kywdpos: {
         StrLen(*dp0) = 4;
         StrLoc(*dp0) = "&pos";
         }

      kywdsubj: {
         StrLen(*dp0) = 8;
         StrLoc(*dp0) = "&subject";
         }

      default:
         if (Offset(*dp1) == 0) {
            /*
             * Must be a named variable.
             */
            dp = VarLoc(*dp1);		 /* get address of variable */
            if (InRange(globals,dp,eglobals)) {
               *dp0 = gnames[dp - globals];		/* global */
	       return GlobalName;
	       }
            else if (InRange(statics,dp,estatics)) {
               i = dp - statics - proc->fstatic;	/* static */
               if (i < 0 || i >= proc->nstatic)
                  syserr("name: unreferencable static variable");
               i += abs((int)proc->nparam) + abs((int)proc->ndynam);
               *dp0 = proc->lnames[i];
	       return StaticName;
               }
            else if (InRange(arg1, dp, &arg1[abs((int)proc->nparam)])) {
               *dp0 = proc->lnames[dp - arg1];          /* argument */
	       return ParamName;
	       }
            else if (InRange(loc1, dp, &loc1[proc->ndynam])) {
               *dp0 = proc->lnames[dp - loc1 + abs((int)proc->nparam)];
	       return LocalName;
               }
            else
               syserr("name: cannot determine variable name");
            }
         else {
            /*
             * Must be an element of a structure.
             */
            blkptr = (union block *)VarLoc(*dp1);
            varptr = (dptr)((word *)VarLoc(*dp1) + Offset(*dp1));
            switch ((int)BlkType(blkptr)) {
               case T_Lelem:		/* list */
                  i = varptr - &blkptr->lelem.lslots[blkptr->lelem.first] + 1;
                  if (i < 1)
                     i += blkptr->lelem.nslots;
                  while (blkptr->lelem.listprev != NULL) {
                     blkptr = blkptr->lelem.listprev;
                     i += blkptr->lelem.nused;
                     }
                  sprintf(sbuf,"L[%ld]", (long)i);
                  i = strlen(sbuf);
                  Protect(StrLoc(*dp0) = alcstr(sbuf,i), return Error);
                  StrLen(*dp0) = i;
                  break;
               case T_Record:		/* record */
                  i = varptr - blkptr->record.fields;
                  proc = &blkptr->record.recdesc->proc;
                  sprintf(sbuf,"%s.%s", StrLoc(proc->recname),
			  StrLoc(proc->lnames[i]));
                  i = strlen(sbuf);
                  Protect(StrLoc(*dp0) = alcstr(sbuf,i), return Error);
                  StrLen(*dp0) = i;
                  break;
               case T_Telem:		/* table */
                  t = keyref(blkptr,dp0);
                  if (t == Error)
                      return Error;
                  break;
               default:		/* none of the above */
                  syserr("name: invalid structure reference");
               }
           }
      }
   return Succeeded;
   }

/*
 * keyref(bp,dp) -- print name of subscripted table
 */
static int keyref(union block *bp, dptr dp)
   {
   char *s, *s2;
   char sbuf[100];			/* buffer; might be too small */
   int len;

   if (getimage(&(bp->telem.tref),dp) == Error)
      return Error;

   /*
    * Allocate space, and copy the image surrounded by "table_n[" and "]"
    */
   s2 = StrLoc(*dp);
   len = StrLen(*dp);
   strcpy(sbuf, "T[");
   { char * dest = sbuf + strlen(sbuf);
   strncpy(dest, s2, len);
   dest[len] = '\0';
   }
   strcat(sbuf, "]");
   len = strlen(sbuf);
   Protect(s = alcstr(sbuf, len), return Error);
   StrLoc(*dp) = s;
   StrLen(*dp) = len;
   return Succeeded;
   }

/*
 * cotrace -- a co-expression context switch; produce a trace message.
 */
void cotrace(struct b_coexpr *ccp, struct b_coexpr *ncp,
   int swtch_typ, dptr valloc)
   {
   struct b_proc *proc;
   inst t_ipc;

   --k_trace;

   /*
    * Compute the ipc of the instruction causing the context switch.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   proc = (struct b_proc *)BlkLoc(*glbl_argp);
   showlevel(k_level);
   putstr(stderr, &proc->pname);
   fprintf(stderr,"; co-expression_%ld ", (long)ccp->id);
   switch (swtch_typ) {
      case A_Coact:
         fprintf(stderr,": ");
         outimage(stderr, valloc, 0);
         fprintf(stderr," @ ");
         break;
      case A_Coret:
         fprintf(stderr,"returned ");
         outimage(stderr, valloc, 0);
         fprintf(stderr," to ");
         break;
      case A_Cofail:
         fprintf(stderr,"failed to ");
         break;
      }
   fprintf(stderr,"co-expression_%ld\n", (long)ncp->id);
   fflush(stderr);
   }

/*
 * showline - print file and line number information.
 */
static void showline(char *f, int l)
   {
   int i;

   i = (int)strlen(f);
   while (i > 13) {
      f++;
      i--;
      }
   if (l > 0)
      fprintf(stderr, "%-13s: %4d  ",f, l);
   else
      fprintf(stderr, "             :       ");
   }

/*
 * showlevel - print "| " n times.
 */
static void showlevel(int n)
   {
   while (n-- > 0) {
      putc('|', stderr);
      putc(' ', stderr);
      }
   }

#include "../h/opdefs.h"

extern struct descrip value_tmp;		/* argument of Op_Apply */
extern struct b_iproc *opblks[];


/*
 * ttrace - show offending expression.
 */
static void ttrace()
   {
   struct b_proc *bp;
   word nargs;
   switch ((int)lastop) {

      case Op_Keywd:
         fprintf(stderr,"bad keyword reference");
         break;

      case Op_Invoke:
         bp = (struct b_proc *)BlkLoc(*xargp);
         nargs = xnargs;
         if (xargp[0].dword == D_Proc)
            putstr(stderr, &(bp->pname));
         else
            outimage(stderr, xargp, 0);
         putc('(', stderr);
         while (nargs--) {
            outimage(stderr, ++xargp, 0);
            if (nargs)
               putc(',', stderr);
            }
         putc(')', stderr);
         break;

      case Op_Toby:
         putc('{', stderr);
         outimage(stderr, ++xargp, 0);
         fprintf(stderr, " to ");
         outimage(stderr, ++xargp, 0);
         fprintf(stderr, " by ");
         outimage(stderr, ++xargp, 0);
         putc('}', stderr);
         break;

      case Op_Subsc:
         putc('{', stderr);
         outimage(stderr, ++xargp, 0);
         putc('[', stderr);
         outimage(stderr, ++xargp, 0);
         putc(']', stderr);
         putc('}', stderr);
         break;

      case Op_Sect:
         putc('{', stderr);
         outimage(stderr, ++xargp, 0);
         putc('[', stderr);
         outimage(stderr, ++xargp, 0);
         putc(':', stderr);
         outimage(stderr, ++xargp, 0);
         putc(']', stderr);
         putc('}', stderr);
         break;

      case Op_Bscan:
         putc('{', stderr);
         outimage(stderr, xargp, 0);
         fputs(" ? ..}", stderr);
         break;

      case Op_Coact:
         putc('{', stderr);
         outimage(stderr, ++xargp, 0);
         fprintf(stderr, " @ ");
         outimage(stderr, ++xargp, 0);
         putc('}', stderr);
         break;

      case Op_Apply:
         outimage(stderr, xargp++, 0);
         fprintf(stderr," ! ");
         outimage(stderr, &value_tmp, 0);
         break;

      case Op_Create:
         fprintf(stderr,"{create ..}");
         break;

      case Op_Field:
         putc('{', stderr);
         outimage(stderr, ++xargp, 0);
         fprintf(stderr, " . ");
	 ++xargp;
	 if (IntVal(*xargp) == -1)
            fprintf(stderr, "field");
	 else
            fprintf(stderr, "%s", StrLoc(fnames[IntVal(*xargp)]));
         putc('}', stderr);
         break;

      case Op_Limit:
         fprintf(stderr, "limit counter: ");
         outimage(stderr, xargp, 0);
         break;

      case Op_Llist:
         fprintf(stderr,"[ ... ]");
         break;

      default:

         bp = (struct b_proc *)opblks[lastop];
         nargs = abs((int)bp->nparam);
         putc('{', stderr);
         if (lastop == Op_Bang || lastop == Op_Random)
            goto oneop;
         if (abs((int)bp->nparam) >= 2) {
            outimage(stderr, ++xargp, 0);
            putc(' ', stderr);
            putstr(stderr, &(bp->pname));
            putc(' ', stderr);
	    }
         else
oneop:
         putstr(stderr, &(bp->pname));
         outimage(stderr, ++xargp, 0);
         putc('}', stderr);
      }

   if (ipc.opnd != NULL)
      fprintf(stderr, " from line %d in %s", findline(ipc.opnd),
         findfile(ipc.opnd));
   putc('\n', stderr);
   fflush(stderr);
   }


/*
 * ctrace - procedure named s is being called with nargs arguments, the first
 *  of which is at arg; produce a trace message.
 */
void ctrace(dptr dp, int nargs, dptr arg)
   {

   showline(findfile(ipc.opnd), findline(ipc.opnd));
   showlevel(k_level);
   putstr(stderr, dp);
   putc('(', stderr);
   while (nargs--) {
      outimage(stderr, arg++, 0);
      if (nargs)
         putc(',', stderr);
      }
   putc(')', stderr);
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * rtrace - procedure named s is returning *rval; produce a trace message.
 */

void rtrace(dptr dp, dptr rval)
   {
   inst t_ipc;

   /*
    * Compute the ipc of the return instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, dp);
   fprintf(stderr, " returned ");
   outimage(stderr, rval, 0);
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * failtrace - procedure named s is failing; produce a trace message.
 */

void failtrace(dptr dp)
   {
   inst t_ipc;

   /*
    * Compute the ipc of the fail instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, dp);
   fprintf(stderr, " failed");
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * strace - procedure named s is suspending *rval; produce a trace message.
 */

void strace(dptr dp, dptr rval)
   {
   inst t_ipc;

   /*
    * Compute the ipc of the suspend instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, dp);
   fprintf(stderr, " suspended ");
   outimage(stderr, rval, 0);
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * atrace - procedure named s is being resumed; produce a trace message.
 */

void atrace(dptr dp)
   {
   inst t_ipc;

   /*
    * Compute the ipc of the instruction causing resumption.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, dp);
   fprintf(stderr, " resumed");
   putc('\n', stderr);
   fflush(stderr);
   }

/*
 * coacttrace -- co-expression is being activated; produce a trace message.
 */
void coacttrace(struct b_coexpr *ccp, struct b_coexpr *ncp)
   {
   struct b_proc *bp;
   inst t_ipc;

   bp = (struct b_proc *)BlkLoc(*glbl_argp);
   /*
    * Compute the ipc of the activation instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, &(bp->pname));
   fprintf(stderr,"; co-expression_%ld : ", (long)ccp->id);
   outimage(stderr, (dptr)(sp - 3), 0);
   fprintf(stderr," @ co-expression_%ld\n", (long)ncp->id);
   fflush(stderr);
   }

/*
 * corettrace -- return from co-expression; produce a trace message.
 */
void corettrace(struct b_coexpr *ccp, struct b_coexpr *ncp)
   {
   struct b_proc *bp;
   inst t_ipc;

   bp = (struct b_proc *)BlkLoc(*glbl_argp);
   /*
    * Compute the ipc of the coret instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, &(bp->pname));
   fprintf(stderr,"; co-expression_%ld returned ", (long)ccp->id);
   outimage(stderr, (dptr)(&ncp->es_sp[-3]), 0);
   fprintf(stderr," to co-expression_%ld\n", (long)ncp->id);
   fflush(stderr);
   }

/*
 * cofailtrace -- failure return from co-expression; produce a trace message.
 */
void cofailtrace(struct b_coexpr *ccp, struct b_coexpr *ncp)
   {
   struct b_proc *bp;
   inst t_ipc;

   bp = (struct b_proc *)BlkLoc(*glbl_argp);
   /*
    * Compute the ipc of the cofail instruction.
    */
   t_ipc.op = ipc.op - 1;
   showline(findfile(t_ipc.opnd), findline(t_ipc.opnd));
   showlevel(k_level);
   putstr(stderr, &(bp->pname));
   fprintf(stderr,"; co-expression_%ld failed to co-expression_%ld\n",
      (long)ccp->id, (long)ncp->id);
   fflush(stderr);
   }

/*
 * Service routine to display variables in given number of
 *  procedure calls to file f.
 */

int xdisp(struct pf_marker *fp, dptr dp, int count, FILE *f)
   {
   register dptr np;
   register int n;
   struct b_proc *bp;
   word nglobals, *indices;

   while (count--) {		/* go back through 'count' frames */
      if (fp == NULL)
         break;       /* needed because &level is wrong in co-expressions */
      bp = (struct b_proc *)BlkLoc(*dp++); /* get addr of procedure block */

      /*
       * Print procedure name.
       */
      putstr(f, &(bp->pname));
      fprintf(f, " local identifiers:\n");

      /*
       * Print arguments.
       */
      np = bp->lnames;
      for (n = abs((int)bp->nparam); n > 0; n--) {
         fprintf(f, "   ");
         putstr(f, np);
         fprintf(f, " = ");
         outimage(f, dp++, 0);
         putc('\n', f);
         np++;
         }

      /*
       * Print locals.
       */
      dp = &fp->pf_locals[0];
      for (n = bp->ndynam; n > 0; n--) {
         fprintf(f, "   ");
         putstr(f, np);
         fprintf(f, " = ");
         outimage(f, dp++, 0);
         putc('\n', f);
         np++;
         }

      /*
       * Print statics.
       */
      dp = &statics[bp->fstatic];
      for (n = bp->nstatic; n > 0; n--) {
         fprintf(f, "   ");
         putstr(f, np);
         fprintf(f, " = ");
         outimage(f, dp++, 0);
         putc('\n', f);
         np++;
         }
      dp = fp->pf_argp;
      fp = fp->pf_pfp;
      }

   /*
    * Print globals.  Sort names in lexical order using temporary index array.
    */
   nglobals = eglobals - globals;
   indices = (word *)malloc(nglobals * sizeof(word));
   if (indices == NULL)
      return Failed;
   else {
      for (n = 0; n < nglobals; n++)
         indices[n] = n;
      qsort ((char*)indices, (int)nglobals, sizeof(word), (int (*)())glbcmp);
      fprintf(f, "\nglobal identifiers:\n");
      for (n = 0; n < nglobals; n++) {
         fprintf(f, "   ");
         putstr(f, &gnames[indices[n]]);
         fprintf(f, " = ");
         outimage(f, &globals[indices[n]], 0);
         putc('\n', f);
         }
      fflush(f);
      free((pointer)indices);
      }
   return Succeeded;
   }

/*
 * glbcmp - compare the names of two globals using their temporary indices.
 */
static int glbcmp(char *pi, char *pj)
   {
   register word i = *(word *)pi;
   register word j = *(word *)pj;
   return lexcmp(&gnames[i], &gnames[j]);
   }

